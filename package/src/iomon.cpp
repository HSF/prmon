// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

#include "iomon.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "utils.h"

#define MONITOR_NAME "iomon"

// Define this to be 1 to activate the artificial
// suppression of i/o values so that the recovery
// of stats from the max values is checked
#define IOMON_TEST 0

// Constructor; uses RAII pattern to be valid
// after construction
iomon::iomon() : io_stats{} {
  log_init(MONITOR_NAME);
#undef MONITOR_NAME
  for (const auto& param : params) {
    io_stats.emplace(param.get_name(), prmon::monitored_value(param, true));
  }
}

void iomon::update_stats(const std::vector<pid_t>& pids,
                         const std::string read_path) {
  prmon::monitored_value_map io_stat_update{};
  for (const auto& value : io_stats) {
    io_stat_update[value.first] = 0L;
  }
  std::string param{};
  prmon::mon_value value{};
  for (const auto pid : pids) {
    std::stringstream io_fname{};
    io_fname << read_path << "/proc/" << pid << "/io" << std::ends;
    std::ifstream proc_io{io_fname.str()};
    while (proc_io) {
      proc_io >> param >> value;
      if (proc_io && param.size() > 0) {
        param.erase(param.size() - 1);  // Chop off training ":"
        auto element = io_stat_update.find(param);
        if (element != io_stat_update.end()) element->second += value;
      }
    }
  }

#if IOMON_TEST == 1
  long stat_counter = 0;

  for (auto& stat : io_stat_update) {
    // This code block randomly suppresses io stat values
    // to test recovery from the peak measured values
    auto t = time(NULL);
    auto m = (t + stat_counter) % 4;
    std::stringstream strm;
    strm << stat_counter << " " << t << " " << m;
    debug(strm.str());
    if (m == 0) stat.second = 0;
    ++stat_counter;
  }
#endif

  for (auto& value : io_stats)
    value.second.set_value(io_stat_update[value.first]);
}

// Return the counters
prmon::monitored_value_map const iomon::get_text_stats() {
  prmon::monitored_value_map io_stat_map{};
  for (const auto& value : io_stats) {
    io_stat_map[value.first] = value.second.get_value();
  }
  return io_stat_map;
}

// Same for JSON
prmon::monitored_value_map const iomon::get_json_total_stats() {
  return get_text_stats();
}

// For JSON averages, divide by elapsed time
prmon::monitored_average_map const iomon::get_json_average_stats(
    unsigned long long elapsed_clock_ticks) {
  prmon::monitored_average_map io_average_stats{};
  for (const auto& io_param : io_stats) {
    io_average_stats[io_param.first] =
        prmon::avg_value(io_param.second.get_value() * sysconf(_SC_CLK_TCK)) /
        elapsed_clock_ticks;
  }
  return io_average_stats;
}

// Return the parameter list
prmon::parameter_list const iomon::get_parameter_list() { return params; }
// Collect related hardware information
void const iomon::get_hardware_info(nlohmann::json& hw_json) { return; }

void const iomon::get_unit_info(nlohmann::json& unit_json) {
  prmon::fill_units(unit_json, params);
  return;
}
