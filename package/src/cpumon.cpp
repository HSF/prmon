// Copyright (C) CERN, 2018

#include "cpumon.h"

#include <string.h>
#include <unistd.h>

#include <iostream>

const static std::vector<std::string> default_cpu_params{
    "utime", "stime", "cutime", "cstime"};

const static unsigned int fname_size{64};
const static unsigned int stat_buf_size{2048};

const static float inv_clock_ticks = 1. / sysconf(_SC_CLK_TCK);

// Constructor; uses RAII pattern to be valid
// after construction
cpumon::cpumon() : cpu_stats{}, pid_cpu_stats{} {
  for (const auto& cpu_param : default_cpu_params) cpu_stats[cpu_param] = 0;
}

void cpumon::update_stats(const std::vector<pid_t>& pids) {
  // TODO use ifstream, but need to read known strings more flexibly
  // and tie to the vector of parameters being parsed
  char stat_fname_buffer[fname_size];
  char line_buffer[stat_buf_size];
  for (const auto pid : pids) {
    snprintf(stat_fname_buffer, fname_size, "/proc/%d/stat", pid);
    FILE* file3 = fopen(stat_fname_buffer, "r");
    fgets(line_buffer, 2048, file3);
    auto tsbuffer = strchr(line_buffer, ')');
    unsigned long long utime, stime, cutime, cstime;
    if (sscanf(tsbuffer + 2,
               "%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %80llu %80llu "
               "%80llu %80llu",
               &utime, &stime, &cutime, &cstime)) {
      pid_cpu_stats[pid]["utime"] = utime;
      pid_cpu_stats[pid]["stime"] = stime;
      pid_cpu_stats[pid]["cutime"] = cutime;
      pid_cpu_stats[pid]["cstime"] = cstime;
    }
    fclose(file3);
  }

  // Now that per-PID stats are known, calculate the totals
  for (auto& cpu_stat: cpu_stats)
    cpu_stat.second = 0;
  for (const auto& pid_stats : pid_cpu_stats) {
    for (const auto& cpu_stat : pid_stats.second) {
      cpu_stats[cpu_stat.first] += cpu_stat.second;
    }
  }
  for (auto& cpu_stat: cpu_stats)
    cpu_stat.second *= inv_clock_ticks;

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
  std::map<std::string, unsigned long long> json_average_stats{};
  return json_average_stats;
}
