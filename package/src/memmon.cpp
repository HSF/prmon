// Copyright (C) CERN, 2020

#include "memmon.h"
#include "utils.h"

#include <string.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <regex>

// Constructor; uses RAII pattern to be valid
// after construction
memmon::memmon() : mem_params{prmon::default_memory_params}, iterations{0} {
  for (const auto& mem_param : mem_params) {
    mem_stats[mem_param] = 0;
    mem_peak_stats[mem_param] = 0;
    mem_average_stats[mem_param] = 0;
    mem_total_stats[mem_param] = 0;
  }
}

void memmon::update_stats(const std::vector<pid_t>& pids) {
  for (auto& stat : mem_stats) stat.second = 0;

  std::string key_str{}, value_str{};
  for (const auto pid : pids) {
    std::stringstream smaps_fname{};
    smaps_fname << "/proc/" << pid << "/smaps" << std::ends;
    std::ifstream smap_stat{smaps_fname.str()};
    while (smap_stat) {
      // Read off the potentially interesting "key: value", then discard
      // the rest of the line
      smap_stat >> key_str >> value_str;
      smap_stat.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
      if (smap_stat) {
        if (key_str == "Size:") {
          mem_stats["vmem"] += std::stol(value_str);
        } else if (key_str == "Pss:") {
          mem_stats["pss"] += std::stol(value_str);
        } else if (key_str == "Rss:") {
          mem_stats["rss"] += std::stol(value_str);
        } else if (key_str == "Swap:") {
          mem_stats["swap"] += std::stol(value_str);
        }
      }
    }
  }

  // Update the statistics with the new snapshot values
  ++iterations;
  for(const auto& mem_param : mem_params) {
    if (mem_stats[mem_param] > mem_peak_stats[mem_param])
      mem_peak_stats[mem_param] = mem_stats[mem_param];
    mem_total_stats[mem_param] += mem_stats[mem_param];
    mem_average_stats[mem_param] = double(mem_total_stats[mem_param]) / iterations;
  }
}

// Return the summed counters
std::map<std::string, unsigned long long> const memmon::get_text_stats() {
  return mem_stats;
}

// For JSON totals return peaks
std::map<std::string, unsigned long long> const memmon::get_json_total_stats() {
  return mem_peak_stats;
}

// Average values are calculated already for us based on the iteration
// count
std::map<std::string, double> const memmon::get_json_average_stats(
    unsigned long long elapsed_clock_ticks) {
  return mem_average_stats;
}

// Collect related hardware information
void const memmon::get_hardware_info(nlohmann::json& hw_json) {

  // Read some information from /proc/meminfo
  std::ifstream memInfoFile{"/proc/meminfo"};
  if(!memInfoFile.is_open()) {
    std::cerr << "Failed to open /proc/meminfo" << std::endl;
    return;
  }

  // Metrics to read from the input
  std::vector<std::string> metrics{"MemTotal"};

  // Loop over the file
  std::string line;
  while (std::getline(memInfoFile,line)) {
    if (line.empty()) continue;
    size_t splitIdx = line.find(":");
    std::string val;
    if (splitIdx != std::string::npos) {
      val = line.substr(splitIdx + 1);
      if (val.empty()) continue;
      for (const auto& metric : metrics) {
        if (line.size() >= metric.size() && line.compare(0, metric.size(), metric) == 0) {
          val = val.substr(0, val.size()-3); // strip the trailing kB
          hw_json["HW"]["mem"][metric] = std::stol(std::regex_replace(val, std::regex("^\\s+|\\s+$"), ""));
        } // end of metric check
      } // end of populating metrics
    } // end of seperator check
  } // end of reading memInfoFile

  // Close the file
  memInfoFile.close();

  return;
}
