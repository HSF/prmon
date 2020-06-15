// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

// Generic header for utilities used by the prmon
// mointors

#ifndef PRMON_UTILS_H
#define PRMON_UTILS_H 1

#include <unistd.h>

#include <string>
#include <vector>

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

// Default parameter lists for monitor classes
const static std::vector<std::string> default_cpu_params{"utime", "stime"};
const static std::vector<std::string> default_network_if_params{
    "rx_bytes", "rx_packets", "tx_bytes", "tx_packets"};
const static std::vector<std::string> default_wall_params{"wtime"};
const static std::vector<std::string> default_memory_params{"vmem", "pss",
                                                            "rss", "swap"};
const static std::vector<std::string> default_io_params{
    "rchar", "wchar", "read_bytes", "write_bytes"};
const static std::vector<std::string> default_count_params{"nprocs",
                                                           "nthreads"};

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

}  // namespace prmon

#endif  // PRMON_UTILS_H
