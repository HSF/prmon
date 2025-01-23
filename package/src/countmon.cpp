// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

#include "countmon.h"

#include <string.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "utils.h"

#define MONITOR_NAME "countmon"

// Constructor; uses RAII pattern to be valid
// after construction
countmon::countmon() {
  log_init(MONITOR_NAME);
#undef MONITOR_NAME
  for (const auto& param : params) {
    count_stats.emplace(param.get_name(), prmon::monitored_value(param));
  }
}

void countmon::update_stats(const std::vector<pid_t>& pids,
                            const std::string read_path) {
  prmon::monitored_value_map count_stat_update{};
  for (const auto& stat : count_stats) {
    count_stat_update[stat.first] = 0L;
  }

  std::vector<std::string> stat_entries{};
  stat_entries.reserve(prmon::stat_count_read_limit + 1);
  std::string tmp_str{};
  for (const auto pid : pids) {
    std::stringstream stat_fname{};
    stat_fname << read_path << "/proc/" << pid << "/stat" << std::ends;
    std::ifstream proc_stat{stat_fname.str()};
    while (proc_stat &&
           stat_entries.size() < prmon::stat_count_read_limit + 1) {
      proc_stat >> tmp_str;
      if (proc_stat) stat_entries.push_back(tmp_str);
    }
    if (stat_entries.size() > prmon::stat_count_read_limit) {
      count_stat_update["nprocs"] += 1L;
      count_stat_update["nthreads"] +=
          std::stol(stat_entries[prmon::num_threads]);
    }
    stat_entries.clear();
  }

  // Update the statistics with the new snapshot values
  for (auto& value : count_stats)
    value.second.set_value(count_stat_update[value.first]);
}

// Return the counter map
prmon::monitored_value_map const countmon::get_text_stats() {
  prmon::monitored_value_map count_stat_map{};
  for (const auto& value : count_stats) {
    count_stat_map[value.first] = value.second.get_value();
  }
  return count_stat_map;
}

// For JSON return the peaks
prmon::monitored_value_map const countmon::get_json_total_stats() {
  prmon::monitored_value_map count_max_stat_map{};
  for (const auto& value : count_stats) {
    count_max_stat_map[value.first] = value.second.get_max_value();
  }
  return count_max_stat_map;
}

// And the averages
prmon::monitored_average_map const countmon::get_json_average_stats(
    unsigned long long elapsed_clock_ticks) {
  prmon::monitored_average_map count_avg_stat_map{};
  for (const auto& value : count_stats) {
    count_avg_stat_map[value.first] = value.second.get_average_value();
  }
  return count_avg_stat_map;
}

// Return the parameter list
prmon::parameter_list const countmon::get_parameter_list() { return params; }

// Collect related hardware information
void const countmon::get_hardware_info(nlohmann::json& hw_json) { return; }

void const countmon::get_unit_info(nlohmann::json& unit_json) {
  prmon::fill_units(unit_json, params);
  return;
}
