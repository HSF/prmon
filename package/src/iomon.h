// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

// I/O monitoring class
//

#ifndef PRMON_IOMON_H
#define PRMON_IOMON_H 1

#include <map>
#include <string>
#include <vector>

#include "Imonitor.h"
#include "parameter.h"
#include "registry.h"

class iomon final : public Imonitor {
 private:
  // Setup the parameters to monitor here
  const prmon::parameter_list params = {{"rchar", "B", "B/s"},
                                        {"wchar", "B", "B/s"},
                                        {"read_bytes", "B", "B/s"},
                                        {"write_bytes", "B", "B/s"}};

  // Which network io paramters to measure and output key names
  std::vector<std::string> io_params;

  // Container for stats
  std::map<std::string, unsigned long long> io_stats;

 public:
  iomon();

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
REGISTER_MONITOR(Imonitor, iomon, "Monitor input and output activity")

#endif  // PRMON_IOMON_H
