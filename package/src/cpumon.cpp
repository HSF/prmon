// Copyright (C) CERN, 2019

#include "cpuinfo.h"
#include "cpumon.h"
#include "utils.h"

#include <string.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>

// Constructor; uses RAII pattern to be valid
// after construction
cpumon::cpumon(bool store_cpu_freq) : cpu_params{prmon::default_cpu_params}, cpu_stats{},
  m_num_cpus{cpuinfo::get_number_of_cpus()}, m_store_cpu_freq{store_cpu_freq},
  m_iterations{0} {
  if(m_store_cpu_freq) {
    for (int idx = 0; idx < m_num_cpus; ++idx) {
      cpu_params.push_back("CPU" + std::to_string(idx));
    }
  }
  for (const auto& cpu_param : cpu_params) {
    cpu_stats[cpu_param] = 0;
    cpu_peak_stats[cpu_param] = 0;
    cpu_average_stats[cpu_param] = 0;
    cpu_total_stats[cpu_param] = 0;
  }
}

void cpumon::update_stats(const std::vector<pid_t>& pids) {
  for (auto stat : cpu_stats) stat.second = 0;

  std::vector<std::string> stat_entries{};
  stat_entries.reserve(prmon::stat_cpu_read_limit + 1);
  std::string tmp_str{};
  for (const auto pid : pids) {
    std::stringstream stat_fname{};
    stat_fname << "/proc/" << pid << "/stat" << std::ends;
    std::ifstream proc_stat{stat_fname.str()};
    while (proc_stat && stat_entries.size() < prmon::stat_cpu_read_limit + 1) {
      proc_stat >> tmp_str;
      if (proc_stat) stat_entries.push_back(tmp_str);
    }
    if (stat_entries.size() > prmon::stat_cpu_read_limit) {
      cpu_stats["utime"] += std::stol(stat_entries[prmon::utime_pos]) +
                            std::stol(stat_entries[prmon::cutime_pos]);
      cpu_stats["stime"] += std::stol(stat_entries[prmon::stime_pos]) +
                            std::stol(stat_entries[prmon::cstime_pos]);
    }
    stat_entries.clear();
  }
  for (auto& stat : cpu_stats) stat.second /= sysconf(_SC_CLK_TCK);

  // CPU scaling and clock freq information is independent of the processes
  cpu_stats["cpu_scaling"] = cpuinfo::cpu_scaling_info(m_num_cpus);
  if(m_store_cpu_freq) {
    int idx = 0;
    for (auto val : cpuinfo::get_processor_clock_freqs()) {
      cpu_stats["CPU" + std::to_string(idx)] = val;
      idx++;
    }
  }

  // Update the statistics with the new snapshot values
  ++m_iterations;
  for(const auto& cpu_param : cpu_params) {
    if (cpu_stats[cpu_param] > cpu_peak_stats[cpu_param])
      cpu_peak_stats[cpu_param] = cpu_stats[cpu_param];
    cpu_total_stats[cpu_param] += cpu_stats[cpu_param];
    cpu_average_stats[cpu_param] = cpu_total_stats[cpu_param] / m_iterations;
  }
}

// Return the summed counters
std::map<std::string, unsigned long long> const cpumon::get_text_stats() {
  return cpu_stats;
}

// For JSON totals return peaks
std::map<std::string, unsigned long long> const cpumon::get_json_total_stats() {
  return cpu_peak_stats;
}

// Only write out the averages for the CPU freqs
std::map<std::string, unsigned long long> const cpumon::get_json_average_stats(
    unsigned long long elapsed_clock_ticks) {
  std::map<std::string, unsigned long long> local_average_stats{cpu_average_stats};
  local_average_stats.erase("utime");
  local_average_stats.erase("stime");
  local_average_stats.erase("cpu_scaling");
  return local_average_stats;
}
