// Copyright (C) CERN, 2018
//
// CPU monitoring class
//

#ifndef PRMON_CPUMON_H
#define PRMON_CPUMON_H 1

#include <string>
#include <map>
#include <unordered_map>
#include <vector>

#include "Imonitor.h"

class cpumon final : public Imonitor {
 private:
  // Which network cpu paramters to measure and output key names
  std::vector<std::string> cpu_params;
  std::vector<std::string> json_total_keys;
  std::vector<std::string> json_average_keys;

  // Container for total stats
  std::map<std::string, unsigned long long> cpu_stats;

  // Container for per-PID stats (this is required to avoid
  // either multiple counting the same PID or losing stats
  // for children as they exit)
  std::unordered_map<pid_t, std::map<std::string, unsigned long long>> pid_cpu_stats;

  // Helper to map parameters to JSON keys
  inline std::string const json_total_key(std::string param) {
    return std::string("tot_" + param);
  }

  // Helper to map parameters to JSON keys
  inline std::string const json_average_key(std::string param) {
    return std::string("avg_" + param);
  }

 public:
  cpumon();

  void update_stats(const std::vector<pid_t>& pids);

  // These are the stat getter methods which retrieve current statistics
  std::map<std::string, unsigned long long> const get_text_stats();
  std::map<std::string, unsigned long long> const get_json_total_stats();
  std::map<std::string, unsigned long long> const get_json_average_stats(time_t elapsed);

};

#endif  // PRMON_CPUMON_H
