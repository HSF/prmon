// Copyright (C) CERN, 2018

#include "iomon.h"
#include "utils.h"

#include <fstream>
#include <iostream>
#include <sstream>

const static std::vector<std::string> default_io_params{
    "rchar", "wchar", "read_bytes", "write_bytes"};

const static unsigned int fname_size{64};
const static unsigned int line_buf_size{256};

// Constructor; uses RAII pattern to be valid
// after construction
iomon::iomon() : io_stats{} {
  for (const auto& io_param : default_io_params) io_stats[io_param] = 0;
}

void iomon::update_stats(const std::vector<pid_t>& pids) {
  std::string param{};
  unsigned long long value{};
  for (auto& stat: io_stats)
    stat.second = 0;
  for (const auto pid : pids) {
    std::stringstream io_fname{};
    io_fname << "/proc/" << pid << "/io" << std::ends;
    std::ifstream proc_io{io_fname.str()};
    while(proc_io) {
      proc_io >> param >> value;
      if (proc_io && param.size() > 0) {
        param.erase(param.size()-1); // Chop off training ":"
        auto element = io_stats.find(param);
        if (element != io_stats.end())
          element->second += value;
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
std::map<std::string, unsigned long long> const iomon::get_json_average_stats(
    unsigned long long elapsed_clock_ticks) {
  std::map<std::string, unsigned long long> json_average_stats{};
  for (const auto& io_param : io_stats) {
    json_average_stats[io_param.first] =
        (io_param.second * prmon::clock_ticks) / elapsed_clock_ticks;
  }
  return json_average_stats;
}
