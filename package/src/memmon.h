// Copyright (C) CERN, 2018
//
// Memory monitoring class
//

#ifndef PRMON_MEMMON_H
#define PRMON_MEMMON_H 1

#include <string>
#include <map>
#include <vector>

#include "Imonitor.h"

class memmon final : public Imonitor {
 private:
  // Which network memory parameters to measure and output key names
  std::vector<std::string> mem_params;

  // Container for total stats
  std::map<std::string, unsigned long long> mem_stats;
  std::map<std::string, unsigned long long> mem_peak_stats;
  std::map<std::string, unsigned long long> mem_average_stats;
  std::map<std::string, unsigned long long> mem_total_stats;

  // Counter for number of iterations
  unsigned long iterations;


 public:
  memmon();

  void update_stats(const std::vector<pid_t>& pids);

  // These are the stat getter methods which retrieve current statistics
  std::map<std::string, unsigned long long> const get_text_stats();
  std::map<std::string, unsigned long long> const get_json_total_stats();
  std::map<std::string, unsigned long long> const get_json_average_stats(unsigned long long elapsed_clock_ticks);

};

#endif  // PRMON_MEMMON_H
