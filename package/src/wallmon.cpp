// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

#include "wallmon.h"

#include <string.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "utils.h"

// Constructor; uses RAII pattern to be valid
// after construction
wallmon::wallmon()
    : walltime_param{},
      start_time_clock_t{0},
      current_clock_t{0},
      got_mother_starttime{false} {
  walltime_param.reserve(params.size());
  for (const auto& param : params) walltime_param.push_back(param.get_name());
}

int wallmon::get_mother_starttime(pid_t mother_pid) {
  std::vector<std::string> stat_entries{};
  stat_entries.reserve(52);
  std::string tmp_str{};

  std::stringstream stat_fname{};
  stat_fname << "/proc/" << mother_pid << "/stat" << std::ends;
  std::ifstream proc_stat{stat_fname.str()};
  while (proc_stat && stat_entries.size() < prmon::uptime_pos + 1) {
    proc_stat >> tmp_str;
    if (proc_stat) stat_entries.push_back(tmp_str);
  }
  if (stat_entries.size() > prmon::uptime_pos) {
    start_time_clock_t = std::stol(stat_entries[prmon::uptime_pos]);
  } else {
    // Some error happened!
    std::clog << "Read only " << stat_entries.size()
              << " from mother PID stat file" << std::endl;
    std::clog << "Stream status of " << stat_fname.str() << " is "
              << (proc_stat ? "good" : "bad") << std::endl;
    return 1;
  }

  return 0;
}

void wallmon::update_stats(const std::vector<pid_t>& pids) {
  if (!got_mother_starttime && pids.size() > 0) {
    if (get_mother_starttime(pids[0])) {
      std::clog << "Error while reading mother starttime" << std::endl;
      return;
    } else {
      got_mother_starttime = true;
    }
  }

  std::ifstream proc_uptime{"/proc/uptime"};
  float uptime_sec{};
  proc_uptime >> uptime_sec;
  if (!proc_uptime) {
    std::clog << "Error while reading /proc/uptime" << std::endl;
    return;
  }
  current_clock_t = uptime_sec * sysconf(_SC_CLK_TCK) - start_time_clock_t;
  walltime_stats["wtime"] = current_clock_t / sysconf(_SC_CLK_TCK);
}

unsigned long long const wallmon::get_wallclock_clock_t() {
  // Just ensure we never return a zero
  return (current_clock_t ? current_clock_t : 1);
}

// Return the summed counters
std::map<std::string, unsigned long long> const wallmon::get_text_stats() {
  return walltime_stats;
}

// Same for JSON
std::map<std::string, unsigned long long> const
wallmon::get_json_total_stats() {
  return walltime_stats;
}

// For walltime there's nothing to return for an average
std::map<std::string, double> const wallmon::get_json_average_stats(
    unsigned long long elapsed_clock_ticks) {
  std::map<std::string, double> empty_average_stats{};
  return empty_average_stats;
}

// Collect related hardware information
void const wallmon::get_hardware_info(nlohmann::json& hw_json) { return; }

void const wallmon::get_unit_info(nlohmann::json& unit_json) {
  prmon::fill_units(unit_json, params);
  return;
}
