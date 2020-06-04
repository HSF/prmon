// Copyright (C) CERN, 2020

#include "cpumon.h"
#include "utils.h"

#include <string.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>

// Constructor; uses RAII pattern to be valid
// after construction
cpumon::cpumon() : cpu_params{prmon::default_cpu_params}, cpu_stats{} {
  for (const auto& cpu_param : cpu_params) cpu_stats[cpu_param] = 0;
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
}

// Return the summed counters
std::map<std::string, unsigned long long> const cpumon::get_text_stats() {
  return cpu_stats;
}

// Same for JSON
std::map<std::string, unsigned long long> const cpumon::get_json_total_stats() {
  return cpu_stats;
}

// For CPU time there's nothing to return for an average
std::map<std::string, double> const cpumon::get_json_average_stats(
    unsigned long long elapsed_clock_ticks) {
  std::map<std::string, double> empty_average_stats{};
  return empty_average_stats;
}

// Collect related hardware information
std::map<std::string, std::map<std::string, std::string>> const cpumon::get_hardware_info() {
  std::map<std::string, std::map<std::string, std::string>> result{};

  // Read some information from /proc/cpuinfo
  std::ifstream cpuInfoFile{"/proc/cpuinfo"};
  if(!cpuInfoFile.is_open()) {
    std::cerr << "Failed to open /proc/cpuinfo" << std::endl;
    return result;
  }

  // Metrics to read from the input
  std::vector<std::string> metrics{"processor", "model name"};
  unsigned int nCPU = 0;

  // Loop over the file
  std::string line;
  while (std::getline(cpuInfoFile,line)) {
    if (line.empty()) continue;
    size_t splitIdx = line.find(":");
    std::string val;
    if (splitIdx != std::string::npos) {
      val = line.substr(splitIdx + 1);
      if (val.empty()) continue;
      for (const auto& metric : metrics) {
        if (line.size() >= metric.size() && line.compare(0, metric.size(), metric) == 0) {
          if (metric == "processor") nCPU++;
          else if (result["cpu"][metric].empty()) {
            result["cpu"][metric] = std::regex_replace(val, std::regex("^\\s+|\\s+$"), "");
          }
        } // end of metric check
      } // end of populating metrics
    } // end of seperator check
  } // end of reading cpuInfoFile

  // Fill nCPU
  result["cpu"]["nCPU"] = std::to_string(nCPU);

  // Close the file
  cpuInfoFile.close();

  return result;
}
