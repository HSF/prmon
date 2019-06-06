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

  // Container for total stats
  std::map<std::string, unsigned long long> cpu_stats;
  std::map<std::string, unsigned long long> cpu_peak_stats;
  std::map<std::string, unsigned long long> cpu_average_stats;
  std::map<std::string, unsigned long long> cpu_total_stats;

  // Number of total processors on the machine
  const int m_num_cpus;

  // Store (or not) the cpu clock freq information
  bool m_store_cpu_freq;

  // Counter for number of iterations
  unsigned long m_iterations;

 public:
  cpumon(bool store_cpu_freq);

  void update_stats(const std::vector<pid_t>& pids);

  // These are the stat getter methods which retrieve current statistics
  std::map<std::string, unsigned long long> const get_text_stats();
  std::map<std::string, unsigned long long> const get_json_total_stats();
  std::map<std::string, unsigned long long> const get_json_average_stats(unsigned long long elapsed_clock_ticks);

};

#endif  // PRMON_CPUMON_H
