// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

// Power monitoring class

#ifndef PRMON_POWERMON_H
#define PRMON_POWERMON_H 1

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "Imonitor.h"
#include "MessageBase.h"
#include "parameter.h"
#include "registry.h"
#include <cstdlib>

class powermon final : public Imonitor, public MessageBase {
 private:
  const prmon::parameter_list params = {
      {"dcmi", "W",  "DCMI power consumption"},
      {"ipmi", "W",  "IPMI power consumption"},
      {"hwmon", "W", "HWMON power consumption"},
      {"rapl_1",  "J", "RAPL package 1 energy in joules"},
      {"rapl_2",  "J", "RAPL package 2 energy in joules"}
  };

  // Default urls to look at
  const std::vector<std::string> default_metrics_urls{
      "http://localhost:9100/metrics",
      "http://localhost:9290/metrics"
  };
  std::vector<std::string> metrics_urls;

  // Directories to look at
  const std::vector<std::string> default_metrics_dirs{
    "/power/ipmi.prom"
  };
  std::vector<std::string> metrics_dirs;

  // Source of metrics, decided first time data is found
  enum class Source {Undecided, Http, Files};
  Source source{Source::Undecided};

//   // Check and choose data source
//   void source_check();

  // Extract lines from the Prometheus text output for each metric type
  void collect_lines(const std::string& body,
                   std::vector<std::string>& dcmi,
                   std::vector<std::string>& rapl,
                   std::vector<std::string>& ipmi,
                   std::vector<std::string>& hwmon) const;
  
  bool valid{false}; // To disable monitor if all ports fail or no metric is read

  prmon::monitored_list power_stats;

  std::string fetch_metrics(const std::string& url);
  double parse_metric_line(const std::string& line,
                           const std::string& metric_name) const;
  
 public:
  powermon();
  ~powermon();

  void update_stats(const std::vector<pid_t>& pids,
                    const std::string read_path = "");
  
  // These are the stat getter methods which retrieve current statistics
  prmon::monitored_value_map const get_text_stats();
  prmon::monitored_value_map const get_json_total_stats();
  prmon::monitored_average_map const get_json_average_stats(
      unsigned long long elapsed_clock_ticks);
  prmon::parameter_list const get_parameter_list();

  // Additional function to read from a directory instead of curl
  std::string read_metrics_file(const std::string& read_path = "");
  std::vector<std::string> gather_bodies(const std::string& read_path);

  // This is the hardware information getter that runs once
  void const get_hardware_info(nlohmann::json& hw_json);
  void const get_unit_info(nlohmann::json& unit_json);
  bool const is_valid() { return valid; }
};
REGISTER_MONITOR(Imonitor, powermon, "Monitor power consumption - prometheus")


#endif  // PRMON_POWERMON_H
