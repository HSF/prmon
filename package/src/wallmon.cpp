// Copyright (C) 2018-2021 CERN
// License Apache2 - see LICENCE file

#include "wallmon.h"

#include <string.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "utils.h"

#define MONITOR_NAME "wallmon"

// Constructor; uses RAII pattern to be valid
// after construction
wallmon::wallmon() : got_mother_starttime{false} {
  log_init(MONITOR_NAME);
#undef MONITOR_NAME
  for (const auto& param : params) {
    walltime_stats.emplace(param.get_name(),
                           prmon::monitored_value(param, true));
  }
}

std::pair<int, unsigned long long> wallmon::get_mother_starttime(
    pid_t mother_pid) {
  std::vector<std::string> stat_entries{};
  stat_entries.reserve(52);
  std::string tmp_str{};
  unsigned long long start_time_clock_t;

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
    std::stringstream strm;
    strm << "Read only " << stat_entries.size() << " from mother PID stat file"
         << std::endl;
    strm << "Stream status of " << stat_fname.str() << " is "
         << (proc_stat ? "good" : "bad") << std::endl;
    warning(strm.str());
    return std::pair<int, unsigned long long>{1, 0L};
  }
  return std::pair<int, unsigned long long>{0, start_time_clock_t};
}

void wallmon::update_stats(const std::vector<pid_t>& pids,
                           const std::string read_path) {
  if (!got_mother_starttime && pids.size() > 0) {
    auto code_and_time = get_mother_starttime(pids[0]);
    if (code_and_time.first) {
      warning("Error while reading mother starttime");
      return;
    } else {
      got_mother_starttime = true;
      walltime_stats.at("wtime").set_offset(code_and_time.second /
                                            sysconf(_SC_CLK_TCK));
    }
  }

  std::ifstream proc_uptime{"/proc/uptime"};
  float uptime_sec{};
  proc_uptime >> uptime_sec;
  if (!proc_uptime) {
    warning("Error while reading /proc/uptime");
    return;
  }

  walltime_stats.at("wtime").set_value(uptime_sec);
}

prmon::mon_value const wallmon::get_wallclock_t() {
  // Just ensure we never return a zero
  return (walltime_stats.at("wtime").get_value()
              ? walltime_stats.at("wtime").get_value()
              : 1);
}

// Return the summed counters
prmon::monitored_value_map const wallmon::get_text_stats() {
  prmon::monitored_value_map walltime_stat_map{};
  for (const auto& value : walltime_stats) {
    walltime_stat_map[value.first] = value.second.get_value();
  }
  return walltime_stat_map;
}

// Same for JSON
prmon::monitored_value_map const wallmon::get_json_total_stats() {
  return get_text_stats();
}

// For walltime there's nothing to return for an average
prmon::monitored_average_map const wallmon::get_json_average_stats(
    unsigned long long elapsed_clock_ticks) {
  static const prmon::monitored_average_map empty_average_stats{};
  return empty_average_stats;
}

// Return the parameter list
prmon::parameter_list const wallmon::get_parameter_list() { return params; }

// Collect related hardware information
void const wallmon::get_hardware_info(nlohmann::json& hw_json) { return; }

void const wallmon::get_unit_info(nlohmann::json& unit_json) {
  prmon::fill_units(unit_json, params);
  return;
}
