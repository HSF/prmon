// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

#include "iomon.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "utils.h"

// Define this to be 1 to activate the artificial
// supression of i/o values so that the recovery
// of stats from the max values is checked
#define IOMON_TEST 0

// Constructor; uses RAII pattern to be valid
// after construction
iomon::iomon() : io_params{}, io_stats{} {
  io_params.reserve(params.size());
  for (const auto& param : params) {
    io_params.push_back(param.get_name());
    io_stats[param.get_name()] = 0;
    io_max_stats[param.get_name()] = 0;
  }
}

void iomon::update_stats(const std::vector<pid_t>& pids) {
  std::string param{};
  unsigned long long value{};
  for (auto& stat : io_stats) stat.second = 0;
  // Loop over each PID and update the stats which are stored for all children
  for (const auto pid : pids) {
    std::stringstream io_fname{};
    io_fname << "/proc/" << pid << "/io" << std::ends;
    std::ifstream proc_io{io_fname.str()};
    while (proc_io) {
      proc_io >> param >> value;
      if (proc_io && param.size() > 0) {
        param.erase(param.size() - 1);  // Chop off training ":"
        auto element = io_stats.find(param);
        if (element != io_stats.end()) element->second += value;
      }
    }
  }

#if IOMON_TEST == 1
  long stat_counter = 0;
#endif

  for (auto& stat : io_stats) {
#if IOMON_TEST == 1
    // This code block randomly suppresses io stat values
    // to test recovery from the peak measured values
    auto t = time(NULL);
    auto m = (t + stat_counter) % 4;
    std::cout << stat_counter << " " << t << " " << m << std::endl;
    if (m == 0) stat.second = 0;
    ++stat_counter;
#endif
    if (stat.second < io_max_stats[stat.first]) {
      // Uh oh - somehow the i/o stats dropped so some i/o was lost
      std::clog << "prmon: Warning, detected drop in i/o values for "
                << stat.first << " (" << io_max_stats[stat.first] << " became "
                << stat.second << ")" << std::endl;
      stat.second = io_max_stats[stat.first];
    } else {
      io_max_stats[stat.first] = stat.second;
    }
  }
}

// Return the counters
std::map<std::string, unsigned long long> const iomon::get_text_stats() {
  return io_stats;
}

// Same for JSON
std::map<std::string, unsigned long long> const iomon::get_json_total_stats() {
  return io_stats;
}

// For JSON averages, divide by elapsed time
std::map<std::string, double> const iomon::get_json_average_stats(
    unsigned long long elapsed_clock_ticks) {
  std::map<std::string, double> json_average_stats{};
  for (const auto& io_param : io_stats) {
    json_average_stats[io_param.first] =
        double(io_param.second * sysconf(_SC_CLK_TCK)) / elapsed_clock_ticks;
  }
  return json_average_stats;
}

// Collect related hardware information
void const iomon::get_hardware_info(nlohmann::json& hw_json) { return; }

void const iomon::get_unit_info(nlohmann::json& unit_json) {
  prmon::fill_units(unit_json, params);
  return;
}
