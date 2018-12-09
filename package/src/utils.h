// Generic header for prmon utilities
//

#ifndef PRMON_UTILS_H
#define PRMON_UTILS_H 1

#include <unistd.h>

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
  const static std::vector<std::string> default_memory_params{"vmem", "pss", "rss", "swap"};
  const static std::vector<std::string> default_io_params{
      "rchar", "wchar", "read_bytes", "write_bytes"};
  const static std::vector<std::string> default_count_params{"nprocs", "nthreads"};
}

#endif  // PRMON_UTILS_H
