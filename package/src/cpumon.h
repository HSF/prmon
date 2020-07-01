// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

// CPU monitoring class
//

#ifndef PRMON_CPUMON_H
#define PRMON_CPUMON_H 1

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "Imonitor.h"
#include "parameter.h"
#include "registry.h"

class cpumon final : public Imonitor {
 private:
  // Setup the parameters to monitor here
  const prmon::parameter_list params = {{"utime", "s", ""}, {"stime", "s", ""}};

  // Which network cpu paramters to measure and output key names
  // This will be filled at initialisation, taking the names
  // from the above params
  std::vector<std::string> cpu_params;

  // Container for total stats
  std::map<std::string, unsigned long long> cpu_stats;

 public:
  cpumon();

  void update_stats(const std::vector<pid_t>& pids);

  // These are the stat getter methods which retrieve current statistics
  std::map<std::string, unsigned long long> const get_text_stats();
  std::map<std::string, unsigned long long> const get_json_total_stats();
  std::map<std::string, double> const get_json_average_stats(
      unsigned long long elapsed_clock_ticks);

  // This is the hardware information getter that runs once
  void const get_hardware_info(nlohmann::json& hw_json);

  void const get_unit_info(nlohmann::json& unit_json);
  bool const is_valid() { return true; }
};
REGISTER_MONITOR(Imonitor, cpumon, "Monitor cpu time used")

#endif  // PRMON_CPUMON_H
