// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

#include "countmon.h"

#include <string.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "utils.h"

// Constructor; uses RAII pattern to be valid
// after construction
countmon::countmon()
    : count_params{prmon::default_count_params},
      count_stats{},
      count_average_stats{},
      count_total_stats{},
      iterations{0L} {
  for (const auto& count_param : count_params) {
    count_stats[count_param] = 0;
    count_peak_stats[count_param] = 0;
    count_average_stats[count_param] = 0;
    count_total_stats[count_param] = 0;
  }
}

void countmon::update_stats(const std::vector<pid_t>& pids) {
  for (auto& stat : count_stats) stat.second = 0;

  std::vector<std::string> stat_entries{};
  stat_entries.reserve(prmon::stat_count_read_limit + 1);
  std::string tmp_str{};
  for (const auto pid : pids) {
    std::stringstream stat_fname{};
    stat_fname << "/proc/" << pid << "/stat" << std::ends;
    std::ifstream proc_stat{stat_fname.str()};
    while (proc_stat &&
           stat_entries.size() < prmon::stat_count_read_limit + 1) {
      proc_stat >> tmp_str;
      if (proc_stat) stat_entries.push_back(tmp_str);
    }
    if (stat_entries.size() > prmon::stat_count_read_limit) {
      count_stats["nprocs"] += 1L;
      count_stats["nthreads"] += std::stol(stat_entries[prmon::num_threads]);
    }
    stat_entries.clear();
  }

  // Update the statistics with the new snapshot values
  ++iterations;
  for (const auto& count_param : count_params) {
    if (count_stats[count_param] > count_peak_stats[count_param])
      count_peak_stats[count_param] = count_stats[count_param];
    count_total_stats[count_param] += count_stats[count_param];
    count_average_stats[count_param] =
        double(count_total_stats[count_param]) / iterations;
  }
}

// Return the counters
std::map<std::string, unsigned long long> const countmon::get_text_stats() {
  return count_stats;
}

// For JSON return the peaks
std::map<std::string, unsigned long long> const
countmon::get_json_total_stats() {
  return count_peak_stats;
}

// An the averages
std::map<std::string, double> const countmon::get_json_average_stats(
    unsigned long long elapsed_clock_ticks) {
  return count_average_stats;
}

// Collect related hardware information
void const countmon::get_hardware_info(nlohmann::json& hw_json) { return; }
