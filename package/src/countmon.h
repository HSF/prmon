// Copyright (C) CERN, 2018
//
// Process and thread number monitoring class
//

#ifndef PRMON_COUNTMON_H
#define PRMON_COUNTMON_H 1

#include <string>
#include <map>
#include <unordered_map>
#include <vector>

#include "Imonitor.h"

class countmon final : public Imonitor {
 private:
  // Which network count paramters to measure and output key names
  std::vector<std::string> count_params;

  // Container for total stats
  std::map<std::string, unsigned long long> count_stats;
  std::map<std::string, unsigned long long> count_peak_stats;
  std::map<std::string, unsigned long long> count_average_stats;
  std::map<std::string, unsigned long long> count_total_stats;

  // Counter for number of iterations
  unsigned long iterations;

 public:
  countmon();

  void update_stats(const std::vector<pid_t>& pids);

  // These are the stat getter methods which retrieve current statistics
  std::map<std::string, unsigned long long> const get_text_stats();
  std::map<std::string, unsigned long long> const get_json_total_stats();
  std::map<std::string, unsigned long long> const get_json_average_stats(unsigned long long elapsed_clock_ticks);

};

#endif  // PRMON_COUNTMON_H
