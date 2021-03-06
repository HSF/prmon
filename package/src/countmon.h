// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

// Process and thread number monitoring class
//

#ifndef PRMON_COUNTMON_H
#define PRMON_COUNTMON_H 1

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "Imonitor.h"
#include "MessageBase.h"
#include "parameter.h"
#include "registry.h"

class countmon final : public Imonitor, public MessageBase {
 private:
  // Setup the parameters to monitor here
  const prmon::parameter_list params = {{"nprocs", "1", "1"},
                                        {"nthreads", "1", "1"}};

  // Which network count paramters to measure and output key names
  std::vector<std::string> count_params;

  // Container for total stats
  std::map<std::string, unsigned long long> count_stats;
  std::map<std::string, unsigned long long> count_peak_stats;
  std::map<std::string, double> count_average_stats;
  std::map<std::string, unsigned long long> count_total_stats;

  // Counter for number of iterations
  unsigned long iterations;

 public:
  countmon();

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
REGISTER_MONITOR(Imonitor, countmon, "Monitor number of processes and threads")

#endif  // PRMON_COUNTMON_H
