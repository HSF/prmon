// Copyright (C) 2018-2021 CERN
// License Apache2 - see LICENCE file

#include "memmon.h"

#include <string.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <limits>
#include <regex>
#include <sstream>

#include "utils.h"

#define MONITOR_NAME "memmon"

// Constructor; uses RAII pattern to be valid
// after construction
memmon::memmon() {
  log_init(MONITOR_NAME);
#undef MONITOR_NAME
  for (const auto& param : params) {
    mem_stats.emplace(param.get_name(), prmon::monitored_value(param));
  }
}

void memmon::update_stats(const std::vector<pid_t>& pids,
                          const std::string read_path) {
  prmon::monitored_value_map mem_stat_update{};
  for (const auto& value : mem_stats) {
    mem_stat_update[value.first] = 0L;
  }
  std::string key_str{}, value_str{};
  for (const auto pid : pids) {
    std::stringstream smaps_fname{};
    smaps_fname << read_path << "/proc/" << pid << "/smaps" << std::ends;
    std::ifstream smap_stat{smaps_fname.str()};
    while (smap_stat) {
      // Read off the potentially interesting "key: value", then discard
      // the rest of the line
      smap_stat >> key_str >> value_str;
      smap_stat.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      if (smap_stat) {
        if (key_str == "Size:") {
          mem_stat_update["vmem"] += std::stol(value_str);
        } else if (key_str == "Pss:") {
          mem_stat_update["pss"] += std::stol(value_str);
        } else if (key_str == "Rss:") {
          mem_stat_update["rss"] += std::stol(value_str);
        } else if (key_str == "Swap:") {
          mem_stat_update["swap"] += std::stol(value_str);
        }
      }
    }
  }

  for (auto& value : mem_stats) {
    value.second.set_value(mem_stat_update[value.first]);
  }
}

// Return the summed counters
prmon::monitored_value_map const memmon::get_text_stats() {
  prmon::monitored_value_map mem_stat_map{};
  for (const auto& value : mem_stats) {
    mem_stat_map[value.first] = value.second.get_value();
  }
  return mem_stat_map;
}

// For JSON totals return peaks
prmon::monitored_value_map const memmon::get_json_total_stats() {
  prmon::monitored_value_map mem_max_stat_map{};
  for (const auto& value : mem_stats) {
    mem_max_stat_map[value.first] = value.second.get_max_value();
  }
  return mem_max_stat_map;
}

// Average values are calculated already for us based on the iteration
// count
prmon::monitored_average_map const memmon::get_json_average_stats(
    unsigned long long elapsed_clock_ticks) {
  prmon::monitored_average_map count_avg_stat_map{};
  for (const auto& value : mem_stats) {
    count_avg_stat_map[value.first] = value.second.get_average_value();
  }
  return count_avg_stat_map;
}

// Collect related hardware information
void const memmon::get_hardware_info(nlohmann::json& hw_json) {
  // Read some information from /proc/meminfo
  std::ifstream memInfoFile{"/proc/meminfo"};
  if (!memInfoFile.is_open()) {
    error("Failed to open /proc/meminfo");
    return;
  }

  // Metrics to read from the input
  std::vector<std::string> metrics{"MemTotal"};

  // Loop over the file
  std::string line;
  while (std::getline(memInfoFile, line)) {
    if (line.empty()) continue;
    size_t splitIdx = line.find(":");
    std::string val;
    if (splitIdx != std::string::npos) {
      val = line.substr(splitIdx + 1);
      if (val.empty()) continue;
      for (const auto& metric : metrics) {
        if (line.size() >= metric.size() &&
            line.compare(0, metric.size(), metric) == 0) {
          val = val.substr(0, val.size() - 3);  // strip the trailing kB
          hw_json["HW"]["mem"][metric] =
              std::stol(std::regex_replace(val, std::regex("^\\s+|\\s+$"), ""));
        }  // end of metric check
      }    // end of populating metrics
    }      // end of seperator check
  }        // end of reading memInfoFile

  // Close the file
  memInfoFile.close();

  return;
}

void const memmon::get_unit_info(nlohmann::json& unit_json) {
  prmon::fill_units(unit_json, params);
  return;
}
