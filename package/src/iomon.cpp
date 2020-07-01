// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

#include "iomon.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "utils.h"

// Constructor; uses RAII pattern to be valid
// after construction
iomon::iomon() : io_stats{} {
  io_params.reserve(params.size());
  for (const auto& param : params) {
    io_params.push_back(param.get_name());
    io_stats[param.get_name()] = 0;
  }
}

void iomon::update_stats(const std::vector<pid_t>& pids) {
  std::string param{};
  unsigned long long value{};
  for (auto& stat : io_stats) stat.second = 0;
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
