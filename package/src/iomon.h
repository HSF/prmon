// Copyright (C) CERN, 2018
//
// I/O monitoring class
//

#ifndef PRMON_IOMON_H
#define PRMON_IOMON_H 1

#include <string>
#include <map>
#include <vector>

#include "Imonitor.h"

class iomon final : public Imonitor {
 private:
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
  std::map<std::string, unsigned long long> const get_json_average_stats(time_t elapsed);

};

#endif  // PRMON_IOMON_H
