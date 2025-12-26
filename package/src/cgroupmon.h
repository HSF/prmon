// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

// Cgroup monitoring class for container resource tracking
//

#ifndef PRMON_CGROUPMON_H
#define PRMON_CGROUPMON_H 1

#include <map>
#include <string>
#include <vector>

#include "Imonitor.h"
#include "MessageBase.h"
#include "parameter.h"
#include "registry.h"

// Cgroup version enumeration
enum class CgroupVersion { NONE, V1, V2, HYBRID };

class cgroupmon final : public Imonitor, public MessageBase {
 private:
  // Parameters to monitor from cgroups
  const prmon::parameter_list params = {
      // CPU metrics
      {"cgroup_cpu_user", "us", "us"},
      {"cgroup_cpu_system", "us", "us"},
      {"cgroup_cpu_total", "us", "us"},
      {"cgroup_cpu_throttled", "us", ""},
      {"cgroup_cpu_periods", "1", ""},
      // Memory metrics
      {"cgroup_mem_current", "kB", "kB"},
      {"cgroup_mem_max", "kB", ""},
      {"cgroup_mem_anon", "kB", "kB"},
      {"cgroup_mem_file", "kB", "kB"},
      {"cgroup_mem_kernel", "kB", "kB"},
      {"cgroup_mem_slab", "kB", "kB"},
      {"cgroup_mem_pgfault", "1", "1/s"},
      {"cgroup_mem_pgmajfault", "1", "1/s"},
      // I/O metrics
      {"cgroup_io_read", "B", "B/s"},
      {"cgroup_io_write", "B", "B/s"},
      // Process/thread counts from cgroup
      {"cgroup_nprocs", "1", "1"},
      {"cgroup_nthreads", "1", "1"}};

  // Dynamic monitoring container
  prmon::monitored_list cgroup_stats;

  // Cgroup detection state
  CgroupVersion cgroup_version;
  bool valid;
  std::string cgroup_path;
  std::string cgroup_mount_point;

  // Helper methods for cgroup detection and reading
  CgroupVersion detect_cgroup_version();
  std::string find_cgroup_path(pid_t pid);
  std::string find_cgroup_mount_point();
  
  // Reading methods for different cgroup versions
  void read_cgroup_v1_stats(const std::string& cgroup_path);
  void read_cgroup_v2_stats(const std::string& cgroup_path);
  
  // Parse specific cgroup files
  void parse_cpu_stat_v2(const std::string& path,
                         prmon::monitored_value_map& stats);
  void parse_memory_stat_v2(const std::string& path,
                            prmon::monitored_value_map& stats);
  void parse_io_stat_v2(const std::string& path,
                        prmon::monitored_value_map& stats);
  void parse_cpu_stat_v1(const std::string& path,
                         prmon::monitored_value_map& stats);
  void parse_memory_stat_v1(const std::string& path,
                            prmon::monitored_value_map& stats);
  void parse_io_stat_v1(const std::string& path,
                        prmon::monitored_value_map& stats);
  
  // Helper to read single value files
  unsigned long long read_single_value(const std::string& filepath);

 public:
  cgroupmon();

  void update_stats(const std::vector<pid_t>& pids,
                    const std::string read_path = "");

  // Stat getter methods
  prmon::monitored_value_map const get_text_stats();
  prmon::monitored_value_map const get_json_total_stats();
  prmon::monitored_average_map const get_json_average_stats(
      unsigned long long elapsed_clock_ticks);
  prmon::parameter_list const get_parameter_list();

  // Hardware/unit information
  void const get_hardware_info(nlohmann::json& hw_json);
  void const get_unit_info(nlohmann::json& unit_json);
  bool const is_valid() { return valid; }
};

REGISTER_MONITOR(Imonitor, cgroupmon,
                 "Monitor cgroup (container) resource usage")

#endif  // PRMON_CGROUPMON_H
