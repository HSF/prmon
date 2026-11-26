// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

#include "machmon.h"

#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <tuple>

#include "utils.h"

#define MONITOR_NAME "machmon"

// Constructor; uses RAII pattern to be valid after construction
machmon::machmon() : mach_stats{} {
  log_init(MONITOR_NAME);
#undef MONITOR_NAME
  for (const auto& param : params) {
    mach_stats.emplace(param.get_name(), prmon::monitored_value(param, true));
  }

  // Read USER_HZ once
  clk_tck = sysconf(_SC_CLK_TCK);
  if (clk_tck <= 0) clk_tck = 100; // Default to 100 if sysconf fails

  initialised = (read_proc_stat("", initial_utime, initial_stime, initial_ntime));
  valid = true;
}

// Read the first line of /proc/stat and return {utime, stime, ntime}
bool machmon::read_proc_stat(const std::string& read_path,
                             long& user, long& system, long& nice) {
  const std::string stat_path = read_path + "/proc/stat";
  std::ifstream proc_stat{stat_path};
  if (!proc_stat) {
    debug("Cannot open " + stat_path + ", skipping this iteration");
    return false;
  }
  std::string cpu_label{};
  if (!(proc_stat >> cpu_label >> user >> nice >> system)) {
    debug("Unexpected format in " + stat_path + ", skipping this iteration");
    return false;
  }
  return true;
}

void machmon::update_stats(const std::vector<pid_t>& /*pids*/,
                              const std::string read_path) {
    long utime{}, stime{}, ntime{};
    if (!read_proc_stat(read_path, utime, stime, ntime)) {
      return;
    }

    // Subtract initial snapshot on first read
    if (!initialised) {
      initial_utime = utime;
      initial_stime = stime;
      initial_ntime = ntime;
      initialised = true;
      if (!read_path.empty()) baseline_from_read_path = true;
    }

    mach_stats.at("utime_total").set_value(std::max(0L, utime - initial_utime) / clk_tck);
    mach_stats.at("stime_total").set_value(std::max(0L, stime - initial_stime) / clk_tck);
    mach_stats.at("ntime_total").set_value(std::max(0L, ntime - initial_ntime) / clk_tck);
}

// Return the summed counters
prmon::monitored_value_map const machmon::get_text_stats() {
  prmon::monitored_value_map cpu_stat_map{};
  for (const auto& value : mach_stats) {
    cpu_stat_map[value.first] = value.second.get_value();
  }
  return cpu_stat_map;
}

// Same for JSON
prmon::monitored_value_map const machmon::get_json_total_stats() {
  return machmon::get_text_stats();
}

// No averages for global CPU time either
prmon::monitored_average_map const machmon::get_json_average_stats(
    unsigned long long /*elapsed_clock_ticks*/) {
  static const prmon::monitored_average_map empty_average_stats{};
  return empty_average_stats;
}

// Return the parameter list
prmon::parameter_list const machmon::get_parameter_list() { return params; }

// No extra hardware information needed here
void const machmon::get_hardware_info(nlohmann::json& /*hw_json*/) {
  return;
}

void const machmon::get_unit_info(nlohmann::json& unit_json) {
  prmon::fill_units(unit_json, params);
  return;
}