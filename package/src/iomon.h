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
  std::vector<std::string> json_total_keys;
  std::vector<std::string> json_average_keys;

  // Container for stats
  std::map<std::string, unsigned long long> io_stats;

  // Helper to map parameters to JSON keys
  inline std::string const json_total_key(std::string param) {
    return std::string("tot_" + param);
  }

  // Helper to map parameters to JSON keys
  inline std::string const json_average_key(std::string param) {
    return std::string("avg_" + param);
  }

 public:
  iomon();

  void update_stats(const std::vector<pid_t>& pids);

  // These are the stat getter methods which retrieve current statistics
  std::map<std::string, unsigned long long> const get_text_stats();
  std::map<std::string, unsigned long long> const get_json_total_stats();
  std::map<std::string, unsigned long long> const get_json_average_stats(time_t elapsed);

};

#endif  // PRMON_IOMON_H
