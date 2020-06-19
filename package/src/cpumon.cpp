// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

#include "cpumon.h"

#include <string.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

#include "utils.h"

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
void const cpumon::get_hardware_info(nlohmann::json& hw_json) {
  // Define the command and run it
  const std::vector<std::string> cmd = {"lscpu"};

  auto cmd_result = prmon::cmd_pipe_output(cmd);

  // If the command failed print an error and move on
  if (cmd_result.first) {
    std::cerr << "Failed to execute 'lscpu' to get CPU information (code "
              << cmd_result.first << ")" << std::endl;
    return;
  }

  // Map lscpu names to the desired ones in the JSON
  const std::map<std::string, std::string> metricToName{
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
    for (const auto& metric : metricToName) {
      if (key != metric.first) continue;
      if (isNumber(value))
        hw_json["HW"]["cpu"][metric.second] = std::stoi(value);
      else
        hw_json["HW"]["cpu"][metric.second] = value;
      break;
    }
  }

  return;
}
