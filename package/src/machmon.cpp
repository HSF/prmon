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

  valid = true;
}

// Read the first line of /proc/stat and return {utime, stime, ntime}

static std::tuple<long, long, long> read_proc_stat(const std::string read_path = "") {
  const std::string stat_path = read_path + "/proc/stat";
  std::ifstream proc_stat{stat_path};
  if (!proc_stat) {
    throw std::runtime_error("Cannot open " + stat_path);
  }

  std::string cpu_label{};
  long user{}, nice{}, system{};

  // First line: "cpu  user nice system idle ..."
  if (!(proc_stat >> cpu_label >> user >> nice >> system)) {
    throw std::runtime_error("Unexpected format in " + stat_path);
  }

  return std::make_tuple(user, system, nice);
}

void machmon::update_stats(const std::vector<pid_t>& /*pids*/,
                              const std::string read_path) {
    if (!valid) {
    return;
    }

    long utime, stime, ntime;
    std::tie(utime, stime, ntime) = read_proc_stat(read_path);

    // Subtract initial snapshot on first read
    if (!initialised) {
      initial_utime = utime;
      initial_stime = stime;
      initial_ntime = ntime;
      initialised = true;
    }

    const long clk = sysconf(_SC_CLK_TCK);
    mach_stats.at("utime_total").set_value(std::max(0L, utime - initial_utime) / clk);
    mach_stats.at("stime_total").set_value(std::max(0L, stime - initial_stime) / clk);
    mach_stats.at("ntime_total").set_value(std::max(0L, ntime - initial_ntime) / clk);
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