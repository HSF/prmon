// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

#include "powermon.h"

#include <curl/curl.h>

#include <sstream>
#include <fstream>
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

// Constructor; uses RAII pattern to be valid 
// after construction
powermon::powermon() : power_stats{} {
  log_init(MONITOR_NAME);
#undef MONITOR_NAME
  
  valid = false; // Default value, only set to true if everything works

  // Global init for curl, explicitly
  if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
    error("Failed to initialise libcurl");
    return;
  }

  // Initialize parameters
  for (const auto& param : params) {
    // RAPL values are monotonic, so we set the flag to true. Else, false.
    if (param.get_name() == "rapl_1" || param.get_name() == "rapl_2") {
      power_stats.emplace(param.get_name(), prmon::monitored_value(param, true));
    } else {
      power_stats.emplace(param.get_name(), prmon::monitored_value(param, false));
    }
  }
  
  // Function to check urls/directories and store them
  auto add_entry = [&](const std::string& entry){
    if (entry.empty()) return;
    if (entry.find("://") != std::string::npos)
      metrics_urls.push_back(entry);
    else
      metrics_dirs.push_back(entry);
  };

  // PROMETHEUS_LOCAL_URL: comma separated list of exporter URLs or directories
  // Expected format: "http://localhost:9100/metrics,/node_exporter/ipmi.prom"
  const char* env_string = std::getenv("PROMETHEUS_LOCAL_URL");
  
  // Read through env_string, push each to either url or directory
  if (env_string) {
    unsigned pos{0}, start{0};
    while (true) {
      if (env_string[pos] == ',' || env_string[pos] == '\0') {
        add_entry(std::string(env_string + start, pos - start));
        if (env_string[pos] == '\0') break;
        start = ++pos;
      } else {
        ++pos;
      }
    }
  }

  // Use default urls if no url was provided in the environment variable
  if (metrics_urls.empty()) {
    metrics_urls = default_metrics_urls;
    debug("No exporter URLs provided, using default powermon ports.");
  };

  // Use default dirs if no dir was provided in the environment variable
  if (metrics_dirs.empty()) {
    metrics_dirs = default_metrics_dirs;
    debug("No power directories provided, using default powermon dirs.");
  };

  // Check at least one endpoint answers
  const std::vector<std::string> bodies = gather_bodies("");
  if (bodies.empty()){
    info("Cannot read power metrics from any exporter or directory."
         "powermon disabled.");
    return;
  }

  // Check at least a metric is available
  std::vector<std::string> dcmi, rapl, ipmi, hwmon;
  for (const auto& body : bodies) collect_lines(body, dcmi, rapl, ipmi, hwmon);
  if (dcmi.empty() && rapl.empty() && ipmi.empty() && hwmon.empty()) {
    info("Exporter reachable but none of the dcmi/rapl/ipmi/hwmon metrics "
         "were found. powermon disabled.");
    return;
  }
  
  // If none of the errors are caught, then valid is set to true.
  valid = true;
}

// Destructor: closes libcurl global state
powermon::~powermon() { curl_global_cleanup(); }

// Fetch Prometheus text from the ctt URL. Returns "" on any failure.
std::string powermon::fetch_metrics(const std::string& url) {
  std::string response{};
  CURL* curl = curl_easy_init();
  if (!curl) {
    error("Failed to initialise libcurl, curl_easy_init() returned NULL");
    return {};
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

  // Configure curl options. If any configuration fails, clean and return.
  if (!set_ok(CURLOPT_URL, url.c_str()) ||
      !set_ok(CURLOPT_WRITEFUNCTION, curl_write_cb) ||
      !set_ok(CURLOPT_WRITEDATA, &response) ||
      !set_ok(CURLOPT_MAXFILESIZE_LARGE, kMaxResponseSize) ||
#if LIBCURL_VERSION_NUM >= 0x075500  // 7.85.0
      !set_ok(CURLOPT_PROTOCOLS_STR, "http") ||
#else
      !set_ok(CURLOPT_PROTOCOLS, static_cast<long>(CURLPROTO_HTTP)) ||
#endif
      !set_ok(CURLOPT_CONNECTTIMEOUT, 1L) ||
      !set_ok(CURLOPT_TIMEOUT, 2L) ||
      !set_ok(CURLOPT_PROXY, "") ||
      !set_ok(CURLOPT_NOSIGNAL, 1L)) {
    curl_easy_cleanup(curl);
    return {};
  };

  // Execute curl, check filesize
  CURLcode curl_response = curl_easy_perform(curl);
  if (curl_response == CURLE_FILESIZE_EXCEEDED || curl_response == CURLE_WRITE_ERROR) {
    error("Response from " + url + "exceeds the " + 
          std::to_string(kMaxResponseSize) + "byte limit, discarding it");
    return {};
  }
  return response;
};

// Read Prometheus text from a text file.
std::string powermon::read_metrics_file(const std::string& path){
  std::ifstream f{path};
  if (!f) return {}; // Missing file

  f.seekg(0, std::ios::end);
  const std::streamoff size = f.tellg();
  if (size < 0 || size > kMaxResponseSize) {
    error("File " + path + " exceeds the " + std::to_string(kMaxResponseSize) + 
    " byte limit, ignoring it");
    return {};
  }
  f.seekg(0);

  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

// Collect Prometheus-style text from, in order of priority:
// 1. exporter URLs (PROMETHEUS_LOCAL_URL or default)
// 2. directories from PROMETHEUS_LOCAL_URL
// 3. default directory <read_path>/power
// First source that works is used.
std::vector<std::string> powermon::gather_bodies(const std::string& read_path){
  std::vector<std::string> bodies;
  
  // 1. URLs (skipped if locked to directories)
  if (source != Source::Files) {
    for (const auto& url : metrics_urls) {
      const std::string body = fetch_metrics(url);
      if (!body.empty()) bodies.push_back(body);
    }
    if(!bodies.empty()) {
      if (source == Source::Undecided){
        info("powermon: reading metrics from exporter URLs");
        source = Source::Http;
        return bodies;
      }
    }
    // Locked to HTTP and failed.
    if (source == Source::Http) return bodies;
    debug("powermon: reading from URLs failed");
    }

  // 2. Directories (if URLs failed or directory chosen)
  std::vector<std::string> dirs = metrics_dirs;
  for (const auto& dir : dirs) {
    debug("powermon: testing directory " + dir);
    const std::string body = read_metrics_file(dir);
    if (!body.empty()) bodies.push_back(body);
    if (!bodies.empty()) {
      if (source == Source::Undecided){
        info("powermon: reading metrics from directory " + dir);
        source = Source::Files;
        break;
      }
    } else {
      debug("powermon: directory " + dir + " did not contain any metric or could not be read.");
    }
    
  }
  return bodies;
}

// Parse a single Prometheus line: "metric_name{labels} value timestamp"
// Returns the value if the line starts with metric_name, otherwise -1.0
double powermon::parse_metric_line(const std::string& line,
                                  const std::string& metric_name) const {
  if (line.empty() || line[0] == '#') return -1.0;  // Skip header or empty lines
  if (line.rfind(metric_name, 0) != 0) return -1.0; // Skip lines without metric_name

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
    // Return -1 if val is NaN, Inf, negative or over the maximum
    if (std::isnan(val) || std::isinf(val)) return -1.0; 
    if (val < 0.0) return -1.0;
    if (val >= static_cast<double>(std::numeric_limits<prmon::mon_value>::max())) return -1.0;
    return val;  
  } catch (...) {
    return -1.0;
  }
}

// Extract the rellevant lines from the text body
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
    if (lower.find("ipmi_power_watts") != std::string::npos ||
        lower.find("ipmi_power_current_watts") != std::string::npos)
      ipmi.push_back(line);
    if (lower.find("node_hwmon_power_watt") != std::string::npos ||
        lower.find("node_hwmon_power_average_watt") != std::string::npos)
      hwmon.push_back(line);
  }
}


void powermon::update_stats(const std::vector<pid_t>& /*pids*/,
                           const std::string read_path) {

  // Attempt to read from each metrics_url                    
  const std::vector<std::string> bodies = gather_bodies(read_path);
  if (bodies.empty()) {
    info("No power metrics from any exporter or directory, disabling powermon");
    valid = false;
    return;
  }

  // Collect matching lines from all sources
  std::vector<std::string> dcmi_lines{}, rapl_lines{}, ipmi_lines{}, hwmon_lines{};
  for (const auto& body : bodies) {
    collect_lines(body, dcmi_lines, rapl_lines, ipmi_lines, hwmon_lines);
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
      if (val < 0.0) val = parse_metric_line(ipmi_lines.back(), "ipmi_power_current_watts");
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