// Copyright (C) 2018-2021 CERN
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

  // Dynamic monitoring container for value measurements
  // This will be filled at initialisation, taking the names
  // from the above params
  prmon::monitored_list count_stats;

 public:
  countmon();

  void update_stats(const std::vector<pid_t>& pids);

  // These are the stat getter methods which retrieve current statistics
  prmon::monitored_value_map const get_text_stats();
  prmon::monitored_value_map const get_json_total_stats();
  prmon::monitored_average_map const get_json_average_stats(
      unsigned long long elapsed_clock_ticks);

  // This is the hardware information getter that runs once
  void const get_hardware_info(nlohmann::json& hw_json);
  void const get_unit_info(nlohmann::json& unit_json);
  bool const is_valid() { return true; }
};
REGISTER_MONITOR(Imonitor, countmon, "Monitor number of processes and threads")

#endif  // PRMON_COUNTMON_H
