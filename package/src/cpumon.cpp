// Copyright (C) CERN, 2018

#include "cpumon.h"

#include <string.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>

const static std::vector<std::string> default_cpu_params{
    "utime", "stime"};

const static float inv_clock_ticks = 1. / sysconf(_SC_CLK_TCK);

const size_t utime_pos = 13;
const size_t stime_pos = 14;
const size_t cutime_pos = 15;
const size_t cstime_pos = 16;
const size_t stat_read_limit = 16;

// Constructor; uses RAII pattern to be valid
// after construction
cpumon::cpumon() : cpu_params{default_cpu_params}, cpu_stats{} {
  for (const auto& cpu_param : cpu_params) cpu_stats[cpu_param] = 0;
}

void cpumon::update_stats(const std::vector<pid_t>& pids) {
  for (auto stat : cpu_stats)
    stat.second = 0;

  std::vector<std::string> stat_entries{};
  stat_entries.reserve(stat_read_limit+1);
  std::string tmp_str{};
  for (const auto pid : pids) {
    std::stringstream stat_fname{};
    stat_fname << "/proc/" << pid << "/stat" << std::ends;
    std::ifstream proc_stat{stat_fname.str()};
    while(proc_stat && stat_entries.size() < stat_read_limit+1) {
      proc_stat >> tmp_str;
      if (proc_stat)
        stat_entries.push_back(tmp_str);
    }
    if (stat_entries.size() > stat_read_limit) {
      cpu_stats["utime"] += std::stol(stat_entries[utime_pos]) + std::stol(stat_entries[cutime_pos]);
      cpu_stats["stime"] += std::stol(stat_entries[stime_pos]) + std::stol(stat_entries[cstime_pos]);
    }
    stat_entries.clear();
  }
  for (auto& stat : cpu_stats)
    stat.second *= inv_clock_ticks;
}

// Return the summed counters
std::map<std::string, unsigned long long> const cpumon::get_text_stats() {
  return cpu_stats;
}

// Same for JSON
std::map<std::string, unsigned long long> const cpumon::get_json_total_stats() {
  return cpu_stats;
}

// For CPU time there's nothing to return for CPU
std::map<std::string, unsigned long long> const cpumon::get_json_average_stats(
    time_t elapsed) {
  std::map<std::string, unsigned long long> empty_average_stats{};
  return empty_average_stats;
}
