// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

#include "curlmon.h"

#include <curl/curl.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "utils.h"

#define MONITOR_NAME "curlmon"

// libcurl write, appending data to a std::string
static size_t curl_write_cb(char* ptr, size_t size, size_t nmemb,
                             void* userdata) {
  auto* buf = reinterpret_cast<std::string*>(userdata);
  buf->append(ptr, size * nmemb);
  return size * nmemb;
}

// Initiate curl_stats
curlmon::curlmon() : curl_stats{} {
  log_init(MONITOR_NAME);
#undef MONITOR_NAME
  for (const auto& param : params) {
    curl_stats.emplace(param.get_name(), prmon::monitored_value(param, true));
  }
}

// Fetch Prometheus text from the ctt URL
std::string curlmon::fetch_metrics(const std::string& url) const {
  std::string response{};
  CURL* curl = curl_easy_init();
  if (!curl) throw std::runtime_error("Failed to initialise libcurl");

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK)
    throw std::runtime_error(std::string("curl request failed: ") +
                             curl_easy_strerror(res));
  return response;
}

// Parse a single Prometheus line: "metric_name{labels} value timestamp"
// Returns the value if the line starts with metric_name, otherwise -1.0
double curlmon::parse_metric_line(const std::string& line,
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

  while (value_start < line.size() && std::isspace(line[value_start]))
    ++value_start;

  try {
    return std::stod(line.substr(value_start));
  } catch (...) {
    return -1.0;
  }
}


void curlmon::update_stats(const std::vector<pid_t>& /*pids*/,
                           const std::string /*read_path*/) {
  
  // Collect matching lines from all URLs
  std::vector<std::string> dcmi_lines{};
  std::vector<std::string> rapl_lines{};
  std::vector<std::string> ipmi_lines{};
  std::vector<std::string> hwmon_lines{};

  if (connection_failed) return;

  int failed_urls = 0;
  for (const auto& url : metrics_urls) {
    try {
      const std::string body = fetch_metrics(url);
      std::istringstream stream{body};
      std::string line{};
      while (std::getline(stream, line)) {
        std::string lower = line;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("ipmi_dcmi_power_consumption_watts") != std::string::npos)
          dcmi_lines.push_back(line);
        if (lower.find("node_rapl_package_joules_total") != std::string::npos)
          rapl_lines.push_back(line);
        if (lower.find("ipmi_power_watts") != std::string::npos)
          ipmi_lines.push_back(line);
        if (lower.find("node_hwmon_power_watt") != std::string::npos)
          hwmon_lines.push_back(line);
        else if (lower.find("node_hwmon_power_average_watt") != std::string::npos)
          hwmon_lines.push_back(line);
      }
    } catch (...) {
      ++failed_urls;  // Silencioso: no se loga nada por URL individual
    }
  }

  if (failed_urls == (int)metrics_urls.size()) {
    error("Cannot connect to any exporter endpoint, disabling curlmon");
    connection_failed = true;
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

  try {
    curl_stats.at("dcmi").set_value(
        static_cast<prmon::mon_value>(dcmi));
    curl_stats.at("rapl_1").set_value(
        static_cast<prmon::mon_value>(rapl_1));
    curl_stats.at("rapl_2").set_value(
        static_cast<prmon::mon_value>(rapl_2));
    curl_stats.at("ipmi").set_value(
        static_cast<prmon::mon_value>(ipmi));
    curl_stats.at("hwmon").set_value(
        static_cast<prmon::mon_value>(hwmon));
  } catch (const std::exception& e) {
    error(std::string("Error storing metrics: ") + e.what());
  }
}


// Return the monitored values
prmon::monitored_value_map const curlmon::get_text_stats() {
  prmon::monitored_value_map stat_map{};
  for (const auto& value : curl_stats) {
    stat_map[value.first] = value.second.get_value();
  }
  return stat_map;
}

prmon::monitored_value_map const curlmon::get_json_total_stats() {
  return curlmon::get_text_stats();
}

prmon::monitored_average_map const curlmon::get_json_average_stats(
    unsigned long long /*elapsed_clock_ticks*/) {
  static const prmon::monitored_average_map empty{};
  return empty;
}

prmon::parameter_list const curlmon::get_parameter_list() { return params; }

void const curlmon::get_hardware_info(nlohmann::json& /*hw_json*/) { return; }

void const curlmon::get_unit_info(nlohmann::json& unit_json) {
  prmon::fill_units(unit_json, params);
  return;
}