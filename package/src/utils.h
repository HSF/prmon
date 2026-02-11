// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

// Generic header for utilities used by the prmon monitors
//

#ifndef PRMON_UTILS_H
#define PRMON_UTILS_H 1

#include <signal.h>
#include <unistd.h>

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "parameter.h"

namespace prmon {
// These constants define where in the stat entry from proc
// we find the parameters of interest
const size_t utime_pos = 13;
const size_t stime_pos = 14;
const size_t cutime_pos = 15;
const size_t cstime_pos = 16;
const size_t stat_cpu_read_limit = 16;
const size_t num_threads = 19;
const size_t stat_count_read_limit = 19;
const size_t uptime_pos = 21;

// This is a utility function that executes a command and
// pipes the output back, returning a vector of strings
// for further processing. To make life easier for the caller
// the input is a vector of strings representing the command
// and any arguments (will be passed to execvp()).
//
// The return value is a pair, the first value is an error status:
//  0 - no error
//  1 - child process exit was not zero (including failure from exec())
//  2 - pipe call failed
//  3 - fork call failed
// The second value is the stdout/stderr from the command, as a vector
// of strings, one string per line of output (newlines are stripped)
//
const std::pair<int, std::vector<std::string>> cmd_pipe_output(
    const std::vector<std::string> cmdargs);

// Utility function to add the units' values to the JSON output
// (the implementation is basically identical for all monitors,
//  but as the base class is virtual we use this in each concrete
//  monitor)
const void fill_units(nlohmann::json& unit_json, const parameter_list& params);

// Utility function to check if smaps_rollup is available on this machine
const bool smaps_rollup_exists();

// Utility function to parse a string to uint
unsigned int parse_uint_field(const std::string& s);
}  // namespace prmon

#endif  // PRMON_UTILS_H
