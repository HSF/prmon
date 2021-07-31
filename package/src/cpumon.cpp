// Copyright (C) 2018-2021 CERN
// License Apache2 - see LICENCE file

#include "cpumon.h"

#include <string.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

#include "utils.h"

#define MONITOR_NAME "cpumon"

// Constructor; uses RAII pattern to be valid
// after construction
cpumon::cpumon() : cpu_stats{} {
  log_init(MONITOR_NAME);
#undef MONITOR_NAME
  for (const auto& param : params) {
    cpu_stats.emplace(
        std::make_pair(param.get_name(), prmon::monitored_value(param, true)));
  }
}

void cpumon::update_stats(const std::vector<pid_t>& pids) {
  prmon::monitored_value_map cpu_stat_update{};
  for (const auto& value : cpu_stats) {
    cpu_stat_update[value.first] = 0L;
  }

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
      cpu_stat_update["utime"] += std::stol(stat_entries[prmon::utime_pos]) +
                                  std::stol(stat_entries[prmon::cutime_pos]);
      cpu_stat_update["stime"] += std::stol(stat_entries[prmon::stime_pos]) +
                                  std::stol(stat_entries[prmon::cstime_pos]);
    }
    stat_entries.clear();
  }
  for (auto& value : cpu_stats)
    value.second.set_value(cpu_stat_update[value.first] / sysconf(_SC_CLK_TCK));
}

// Return the summed counters
prmon::monitored_value_map const cpumon::get_text_stats() {
  prmon::monitored_value_map cpu_stat_map{};
  for (const auto& value : cpu_stats) {
    cpu_stat_map[value.first] = value.second.get_value();
  }
  return cpu_stat_map;
}

// Same for JSON
prmon::monitored_value_map const cpumon::get_json_total_stats() {
  return cpumon::get_text_stats();
}

// For CPU time there's nothing to return for an average
prmon::monitored_average_map const cpumon::get_json_average_stats(
    unsigned long long elapsed_clock_ticks) {
  static const prmon::monitored_average_map empty_average_stats{};
  return empty_average_stats;
}

// Collect related hardware information
void const cpumon::get_hardware_info(nlohmann::json& hw_json) {
  // Define the command and run it
  const std::vector<std::string> cmd = {"lscpu"};

  auto cmd_result = prmon::cmd_pipe_output(cmd);

  // If the command failed print an error and move on
  if (cmd_result.first) {
    error("Failed to execute 'lscpu' to get CPU information (code " +
          std::to_string(cmd_result.first) + ")");
    return;
  }

  // Map lscpu names to the desired ones in the JSON
  const std::unordered_map<std::string, std::string> metricToName{
      {"Model name", "ModelName"},
      {"CPU(s)", "CPUs"},
      {"Socket(s)", "Sockets"},
      {"Core(s) per socket", "CoresPerSocket"},
      {"Thread(s) per core", "ThreadsPerCore"}};

  // Useful function to determine if a string is purely a number
  auto isNumber = [](const std::string& s) {
    return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c) {
                           return !std::isdigit(c);
                         }) == s.end();
  };

  // Loop over the output, parse the line, check the key and store the value if
  // requested
  std::string key{}, value{};

  for (const auto& line : cmd_result.second) {
    // Continue on empty line
    if (line.empty()) continue;

    // Tokenize by ":"
    size_t splitIdx = line.find(":");
    if (splitIdx == std::string::npos) continue;

    // Read "key":"value" pairs
    key = line.substr(0, splitIdx);
    value = line.substr(splitIdx + 1);
    if (key.empty() || value.empty()) continue;
    key = std::regex_replace(key, std::regex("^\\s+|\\s+$"), "");
    value = std::regex_replace(value, std::regex("^\\s+|\\s+$"), "");

    // Fill the JSON with the information
    if (metricToName.count(key) == 1) {
      if (isNumber(value))
        hw_json["HW"]["cpu"][metricToName.at(key)] = std::stoi(value);
      else
        hw_json["HW"]["cpu"][metricToName.at(key)] = value;
    }
  }

  return;
}

void const cpumon::get_unit_info(nlohmann::json& unit_json) {
  prmon::fill_units(unit_json, params);
  return;
}
