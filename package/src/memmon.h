// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

// Memory monitoring class
//

#ifndef PRMON_MEMMON_H
#define PRMON_MEMMON_H 1

#include <map>
#include <string>
#include <vector>

#include "Imonitor.h"
#include "MessageBase.h"
#include "parameter.h"
#include "registry.h"

class memmon final : public Imonitor, public MessageBase {
 private:
  // Default parameter list
  // const static std::vector<std::string> default_memory_params{"vmem", "pss",
  // "rss", "swap"};
  const prmon::parameter_list params = {{"vmem", "kB", "kB"},
                                        {"pss", "kB", "kB"},
                                        {"rss", "kB", "kB"},
                                        {"swap", "kB", "kB"}};

  // The input smaps file to be used
  std::string input_filename;

  // Dynamic monitoring container for value measurements
  // This will be filled at initialisation, taking the names
  // from the above params
  prmon::monitored_list mem_stats;

 public:
  memmon();

  void update_stats(const std::vector<pid_t>& pids,
                    const std::string read_path = "");

  // These are the stat getter methods which retrieve current statistics
  std::map<std::string, unsigned long long> const get_text_stats();
  std::map<std::string, unsigned long long> const get_json_total_stats();
  std::map<std::string, double> const get_json_average_stats(
      unsigned long long elapsed_clock_ticks);
  prmon::parameter_list const get_parameter_list();

  // This is the hardware information getter that runs once
  void const get_hardware_info(nlohmann::json& hw_json);
  void const get_unit_info(nlohmann::json& unit_json);
  bool const is_valid() { return true; }

  // Toggle on fast memory monitoring
  void const do_fastmon();
};
REGISTER_MONITOR(Imonitor, memmon, "Monitor memory usage")

#endif  // PRMON_MEMMON_H
