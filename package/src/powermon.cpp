// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

#include "powermon.h"

#include <curl/curl.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <cmath>
#include <limits>
#include <algorithm>
#include <cctype>


#include "utils.h"

#define MONITOR_NAME "powermon"

// Maximum response size (8 MB)
static const curl_off_t kMaxResponseSize = 8 * 1024 * 1024;

// libcurl write, appending data to a std::string
static size_t curl_write_cb(char* ptr, size_t size, size_t nmemb,
                             void* userdata) {
  auto* buf = reinterpret_cast<std::string*>(userdata);
  const size_t new_size = size * nmemb;

  // Return 0 if the new size would exceed the maximum allowed response size
  if (buf->size() + new_size > static_cast<size_t>(kMaxResponseSize)) return 0;
  
  buf->append(ptr, new_size);
  return new_size;
}

// Constructor; uses RAII pattern to be valid after construction
powermon::powermon() : power_stats{} {
  log_init(MONITOR_NAME);
#undef MONITOR_NAME

// Global init for curl, explicitly
  if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
    error("Failed to initialise libcurl");
    return;
  }

  for (const auto& param : params) {
    if (param.get_name() == "rapl_1" || param.get_name() == "rapl_2") {
      
      // RAPL values are monotonic, so we set the flag to true
      power_stats.emplace(param.get_name(), prmon::monitored_value(param, true));
    } else {
      power_stats.emplace(param.get_name(), prmon::monitored_value(param, false));
    }
  }
  valid = false;
  
  // URLs from the environment variable PROMETHEUS_LOCAL_URL, if it exists
  // Expected format: "http://localhost:9100/metrics,http://localhost:9290/metrics"
  const char* env_string = std::getenv("PROMETHEUS_LOCAL_URL");

  if (env_string) {
    unsigned pos{0}, start{0};
    while (true) {
      if (env_string[pos] == ',' || env_string[pos] == '\0') {
        if (pos > start)
          metrics_urls.push_back(std::string(env_string + start, pos - start));
        if (env_string[pos] == '\0') break;
        start = ++pos;
      } else {
        ++pos;
      }
    }
  }

  // Use default values if no url was provided in the environment variable
  if (metrics_urls.empty()) metrics_urls = default_metrics_urls;

  if (metrics_urls.empty()) {
    debug("No exporter URLs provided, powermon disabled");
    return;
  }

  bool reachable = false;

  std::vector<std::string> dcmi, rapl, ipmi, hwmon;
  for (const auto& url : metrics_urls) {
    try {
      collect_lines(fetch_metrics(url), dcmi, rapl, ipmi, hwmon);
      reachable = true;
    } catch (const std::exception& e) {
      debug("powermon: endpoint " + url + " not usable: " + e.what());
    }
  }

  if (!reachable) {
    info("Cannot connect to any exporter endpoint. powermon disabled.");
    return;
  }
  if (dcmi.empty() && rapl.empty() && ipmi.empty() && hwmon.empty()) {
    info("Exporter reachable but none of the dcmi/rapl/ipmi/hwmon metrics "
         "were found. powermon disabled.");
    return;
  }

  valid = true;
}

// Destructor: closes libcurl global state
powermon::~powermon() {
  curl_global_cleanup();
}

// Fetch Prometheus text from the ctt URL
std::string powermon::fetch_metrics(const std::string& url) {
  std::string response{};
  CURL* curl = curl_easy_init();
  if (!curl) {
    error("Failed to initialise libcurl, curl_easy_init() returned NULL");
    return response;
  }

  // Define a function to check if any response is != 0
  CURLcode rc = CURLE_OK;
  auto set_ok = [&](CURLoption option, auto value) {
    rc = curl_easy_setopt(curl, option, value);
    if (rc != CURLE_OK) {
      error("curl_easy_setopt failed: " + std::string(curl_easy_strerror(rc)));
      curl_easy_cleanup(curl);
      valid = false;
      return false;
    };
    return true;
  };

  if (!set_ok(CURLOPT_URL, url.c_str()) ||
      !set_ok(CURLOPT_WRITEFUNCTION, curl_write_cb) ||
      !set_ok(CURLOPT_WRITEDATA, &response) ||
      !set_ok(CURLOPT_MAXFILESIZE_LARGE, kMaxResponseSize) ||
      !set_ok(CURLOPT_PROTOCOLS_STR, "http") ||
      !set_ok(CURLOPT_CONNECTTIMEOUT, 1L) ||
      !set_ok(CURLOPT_TIMEOUT, 2L) ||
      !set_ok(CURLOPT_PROXY, "")) {
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return response;
  };
  return {};
};

// Parse a single Prometheus line: "metric_name{labels} value timestamp"
// Returns the value if the line starts with metric_name, otherwise -1.0
double powermon::parse_metric_line(const std::string& line,
                                  const std::string& metric_name) const {
  if (line.empty() || line[0] == '#') return -1.0;

  if (line.rfind(metric_name, 0) != 0) return -1.0;

  char next = line[metric_name.size()];
  if (next != '{' && next != ' ' && next != '\t') return -1.0;

  size_t value_start = line.find('}');
  if (value_start == std::string::npos)
    value_start = metric_name.size();
  else
    value_start += 1;

  while (value_start < line.size() && std::isspace(static_cast<unsigned char>(line[value_start])))
    ++value_start;

  try {
    const double val = std::stod(line.substr(value_start));
    if (std::isnan(val) || std::isinf(val)) return -1.0; // Return 1 if val is NaN or Inf
    if (val < 0.0) return -1.0;
    if (val >= static_cast<double>(std::numeric_limits<prmon::mon_value>::max())) return -1.0;
    return val;  
  } catch (...) {
    return -1.0;
  }
}


void powermon::collect_lines(const std::string& body,
                            std::vector<std::string>& dcmi,
                            std::vector<std::string>& rapl,
                            std::vector<std::string>& ipmi,
                            std::vector<std::string>& hwmon) const {
  std::istringstream stream{body};
  std::string line{};
  while (std::getline(stream, line)) {
    std::string lower = line;
    std::transform(lower.begin(), lower.end(), lower.begin(),
               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower.find("ipmi_dcmi_power_consumption_watts") != std::string::npos)
      dcmi.push_back(line);
    if (lower.find("node_rapl_package_joules_total") != std::string::npos)
      rapl.push_back(line);
    if (lower.find("ipmi_power_watts") != std::string::npos)
      ipmi.push_back(line);
    if (lower.find("node_hwmon_power_watt") != std::string::npos)
      hwmon.push_back(line);
    else if (lower.find("node_hwmon_power_average_watt") != std::string::npos)
      hwmon.push_back(line);
  }
}


void powermon::update_stats(const std::vector<pid_t>& /*pids*/,
                           const std::string /*read_path*/) {
  // Collect matching lines from all URLs
  std::vector<std::string> dcmi_lines{};
  std::vector<std::string> rapl_lines{};
  std::vector<std::string> ipmi_lines{};
  std::vector<std::string> hwmon_lines{};

  int failed_urls = 0;
  for (const auto& url : metrics_urls) {
    try {
      collect_lines(fetch_metrics(url), dcmi_lines, rapl_lines,
                    ipmi_lines, hwmon_lines);
    } catch (...) {
      ++failed_urls;
    }
  }

  if (failed_urls == (int)metrics_urls.size()) {
    error("Cannot connect to any exporter endpoint, disabling powermon");
    valid = false;
    return;
  }

  // DCMI: sum all matching lines
  double dcmi = 0.0;
  try {
    for (const auto& line : dcmi_lines) {
      double val = parse_metric_line(line, "ipmi_dcmi_power_consumption_watts");
      if (val >= 0.0) dcmi += val;
    }
  } catch (...) { dcmi = 0.0; }

  // RAPL: penultimate line -> rapl_1, last line -> rapl_2
  double rapl_1 = 0.0, rapl_2 = 0.0;
  try {
    if (rapl_lines.size() >= 2) {
      double v1 = parse_metric_line(rapl_lines[rapl_lines.size() - 2],
                                    "node_rapl_package_joules_total");
      double v2 = parse_metric_line(rapl_lines[rapl_lines.size() - 1],
                                    "node_rapl_package_joules_total");
      if (v1 >= 0.0) rapl_1 = v1;
      if (v2 >= 0.0) rapl_2 = v2;
    } else if (rapl_lines.size() == 1) {
      double v = parse_metric_line(rapl_lines[0], "node_rapl_package_joules_total");
      if (v >= 0.0) rapl_1 = v;
    }
  } catch (...) { rapl_1 = rapl_2 = 0.0; }

  // IPMI: last matching line
  double ipmi = 0.0;
  try {
    if (!ipmi_lines.empty()) {
      double val = parse_metric_line(ipmi_lines.back(), "ipmi_power_watts");
      if (val >= 0.0) ipmi = val;
    }
  } catch (...) { ipmi = 0.0; }

  // HWMON: last matching line
  double hwmon = 0.0;
  try {
    if (!hwmon_lines.empty()) {
      double val = parse_metric_line(hwmon_lines.back(), "node_hwmon_power_average_watt");
      if (val >= 0.0) hwmon = val;
    }
  } catch (...) { hwmon = 0.0; }

  // store function to handle value range checking and storing in power_stats
  auto store = [&](const char* key, double value) {
    if (!(value >= 0.0 && value < static_cast<double>(std::numeric_limits<prmon::mon_value>::max()))) {
      error(std::string("Value for ") + key + " is out of range, ignoring");
      value = 0.0;
    }
    power_stats.at(key).set_value(static_cast<prmon::mon_value>(value));
  };

  try {
    store("dcmi", dcmi);
    store("rapl_1", rapl_1);
    store("rapl_2", rapl_2);
    store("ipmi", ipmi);
    store("hwmon", hwmon);
  } catch (const std::exception& e) {
    error(std::string("Error storing metrics: ") + e.what());
  }
}


// Return the monitored values
prmon::monitored_value_map const powermon::get_text_stats() {
  prmon::monitored_value_map stat_map{};
  for (const auto& value : power_stats) {
    stat_map[value.first] = value.second.get_value();
  }
  return stat_map;
}

prmon::monitored_value_map const powermon::get_json_total_stats() {
  return powermon::get_text_stats();
}

prmon::monitored_average_map const powermon::get_json_average_stats(
    unsigned long long /*elapsed_clock_ticks*/) {
  static const prmon::monitored_average_map empty{};
  return empty;
}

prmon::parameter_list const powermon::get_parameter_list() { return params; }

void const powermon::get_hardware_info(nlohmann::json& /*hw_json*/) { return; }

void const powermon::get_unit_info(nlohmann::json& unit_json) {
  prmon::fill_units(unit_json, params);
  return;
}