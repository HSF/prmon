// Copyright (C) CERN, 2018
//
// Wall time monitoring class
//
// This is implemented separately from cputime as for walltime
// there is one more public getter to extract the current wallclock
// time for use in calculating the average JSON stats
//

#ifndef PRMON_WALLMON_H
#define PRMON_WALLMON_H 1

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "Imonitor.h"
#include "registry.h"

class wallmon final : public Imonitor {
 private:
  // Container for total stat
  std::map<std::string, unsigned long long> walltime_stats;

  unsigned long long start_time_clock_t, current_clock_t;

  // Only need to get the mother start time once, so use
  // a bool to say when it's done
  bool got_mother_starttime;
  int get_mother_starttime(pid_t mother_pid);

 public:
  wallmon();

  void update_stats(const std::vector<pid_t>& pids);

  // These are the stat getter methods which retrieve current statistics
  std::map<std::string, unsigned long long> const get_text_stats();
  std::map<std::string, unsigned long long> const get_json_total_stats();
  std::map<std::string, double> const get_json_average_stats(unsigned long long elapsed_clock_ticks);

  // This is the hardware information getter that runs once
  void const get_hardware_info(nlohmann::json& j);

  // Class specific method to retrieve wallclock time in clock ticks
  unsigned long long const get_wallclock_clock_t();
};
REGISTER_MONITOR(Imonitor, wallmon, "Monitor wallclock time")

#endif  // PRMON_WALLMON_H
