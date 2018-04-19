// Copyright (C) CERN, 2018

#include "iomon.h"

#include <iostream>

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
  // TODO use ifstream, but need to read known strings more flexibly
  // and tie to the vector of parameters being parsed
  char io_fname_buffer[fname_size];
  char line_buffer[line_buf_size];
  unsigned long long value;
  for (const auto pid : pids) {
    snprintf(io_fname_buffer, fname_size, "/proc/%d/io", pid);
    FILE* file2 = fopen(io_fname_buffer, "r");
    if (file2 != 0) {
      while (fgets(line_buffer, line_buf_size, file2)) {
        if (sscanf(line_buffer, "rchar: %80llu", &value) == 1)
          io_stats["rchar"] += value;
        if (sscanf(line_buffer, "wchar: %80llu", &value) == 1)
          io_stats["wchar"] += value;
        if (sscanf(line_buffer, "read_bytes: %80llu", &value) == 1)
          io_stats["read_bytes"] += value;
        if (sscanf(line_buffer, "write_bytes: %80llu", &value) == 1)
          io_stats["write_bytes"] += value;
      }
      fclose(file2);
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
    time_t elapsed) {
  std::map<std::string, unsigned long long> json_average_stats{};
  for (const auto& io_param : io_stats) {
    json_average_stats[json_average_key(io_param.first)] =
        io_param.second / elapsed;
  }
  return json_average_stats;
}
