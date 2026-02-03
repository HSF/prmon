// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

#include "cgroupmon.h"

#include <sys/stat.h>
#include <unistd.h>
#include <climits>

#include <fstream>
#include <iostream>
#include <sstream>

#include "utils.h"

#define MONITOR_NAME "cgroupmon"

// Constructor - detect cgroup version and setup monitoring
cgroupmon::cgroupmon()
    : cgroup_version{CgroupVersion::NONE},
      valid{false},
      cgroup_path{},
      cgroup_mount_point{} {
  log_init(MONITOR_NAME);
#undef MONITOR_NAME

  // Initialize monitored values
  for (const auto& param : params) {
    cgroup_stats.emplace(param.get_name(), prmon::monitored_value(param));
  }

  // Detect cgroup version
  cgroup_version = detect_cgroup_version();
  
  if (cgroup_version == CgroupVersion::NONE) {
    info("No cgroup support detected, cgroupmon will be inactive");
    return;
  }

  // Find cgroup mount point
  cgroup_mount_point = find_cgroup_mount_point();
  if (cgroup_mount_point.empty()) {
    warning("Could not find cgroup mount point");
    return;
  }

  valid = true;
  info("Cgroup monitoring enabled (version: " +
       std::string(cgroup_version == CgroupVersion::V2 ? "v2" : 
                   cgroup_version == CgroupVersion::V1 ? "v1" : "hybrid") + ")");
}

// Detect which cgroup version is available
CgroupVersion cgroupmon::detect_cgroup_version() {
  struct stat st;
  
  // Check for cgroup v2
  if (stat("/sys/fs/cgroup/cgroup.controllers", &st) == 0) {
    // Check if v1 also exists (hybrid mode)
    if (stat("/sys/fs/cgroup/cpu", &st) == 0) {
      return CgroupVersion::HYBRID;
    }
    return CgroupVersion::V2;
  }
  
  // Check for cgroup v1
  if (stat("/sys/fs/cgroup/cpu", &st) == 0) {
    return CgroupVersion::V1;
  }
  
  return CgroupVersion::NONE;
}

// Find the cgroup path for a given PID
std::string cgroupmon::find_cgroup_path(pid_t pid) {
  std::stringstream cgroup_file;
  cgroup_file << "/proc/" << pid << "/cgroup";
  
  std::ifstream cgroup_stream(cgroup_file.str());
  if (!cgroup_stream.is_open()) {
    return "";
  }
  
  std::string line;
  // For cgroup v2, look for line starting with "0::"
  // For cgroup v1, look for cpu controller
  while (std::getline(cgroup_stream, line)) {
    if (cgroup_version == CgroupVersion::V2 || 
        cgroup_version == CgroupVersion::HYBRID) {
      if (line.substr(0, 3) == "0::") {
        return line.substr(3);  // Remove "0::" prefix
      }
    }
    
    if (cgroup_version == CgroupVersion::V1) {
      // Look for cpu controller line like "3:cpu,cpuacct:/docker/abc123"
      size_t first_colon = line.find(':');
      size_t second_colon = line.find(':', first_colon + 1);
      if (first_colon != std::string::npos && second_colon != std::string::npos) {
        std::string controllers = line.substr(first_colon + 1, 
                                             second_colon - first_colon - 1);
        if (controllers.find("cpu") != std::string::npos) {
          return line.substr(second_colon + 1);
        }
      }
    }
  }
  
  return "/";  // Root cgroup
}

// Find the cgroup filesystem mount point
std::string cgroupmon::find_cgroup_mount_point() {
  if (cgroup_version == CgroupVersion::V2 || 
      cgroup_version == CgroupVersion::HYBRID) {
    return "/sys/fs/cgroup";
  }
  
  return "/sys/fs/cgroup";
}

// Update statistics from cgroup
void cgroupmon::update_stats(const std::vector<pid_t>& pids,
                             const std::string read_path) {
  if (!valid || pids.empty()) {
    return;
  }

  // Use first PID to determine cgroup path
  // (all processes in the tree should be in the same cgroup)
  std::string base_path = read_path.empty() ? "" : read_path;
  cgroup_path = find_cgroup_path(pids[0]);

  // Construct full path to cgroup directory
  std::string full_cgroup_path = base_path + cgroup_mount_point + cgroup_path;

  try {
    if (cgroup_version == CgroupVersion::V2) {
      read_cgroup_v2_stats(full_cgroup_path);
    } else if (cgroup_version == CgroupVersion::V1) {
      read_cgroup_v1_stats(full_cgroup_path);
    } else if (cgroup_version == CgroupVersion::HYBRID) {
      // Try v2 first, fallback to v1
      read_cgroup_v2_stats(full_cgroup_path);
    }
  } catch (const std::exception& e) {
    debug("Error reading cgroup stats: " + std::string(e.what()));
  }
}

// Read cgroup v2 statistics
void cgroupmon::read_cgroup_v2_stats(const std::string& cgroup_path) {
  prmon::monitored_value_map stats{};
  
  // CPU statistics
  parse_cpu_stat_v2(cgroup_path + "/cpu.stat", stats);
  
  // Memory statistics
  parse_memory_stat_v2(cgroup_path + "/memory.stat", stats);
  
  // Memory current and max
  stats["cgroup_mem_current"] = read_single_value(cgroup_path + "/memory.current") / 1024;
  unsigned long long mem_max = read_single_value(cgroup_path + "/memory.max");
  if (mem_max != ULLONG_MAX) {
    stats["cgroup_mem_max"] = mem_max / 1024;
  }
  
  // I/O statistics
  parse_io_stat_v2(cgroup_path + "/io.stat", stats);
  
  // Process and thread counts
  stats["cgroup_nprocs"] = read_single_value(cgroup_path + "/cgroup.procs");
  stats["cgroup_nthreads"] = read_single_value(cgroup_path + "/cgroup.threads");
  
  // Update monitored values
  for (auto& value : cgroup_stats) {
    if (stats.count(value.first)) {
      value.second.set_value(stats[value.first]);
    }
  }
}

// Read cgroup v1 statistics
void cgroupmon::read_cgroup_v1_stats(const std::string& base_path) {
  prmon::monitored_value_map stats{};
  
  // CPU statistics from cpuacct
  parse_cpu_stat_v1(base_path + "/cpuacct.stat", stats);
  
  // Memory statistics
  parse_memory_stat_v1(base_path + "/memory.stat", stats);
  stats["cgroup_mem_current"] = read_single_value(base_path + "/memory.usage_in_bytes") / 1024;
  stats["cgroup_mem_max"] = read_single_value(base_path + "/memory.limit_in_bytes") / 1024;
  
  // I/O statistics from blkio
  std::string blkio_path = base_path;
  size_t last_slash = blkio_path.find_last_of('/');
  if (last_slash != std::string::npos) {
    blkio_path = "/sys/fs/cgroup/blkio" + blkio_path.substr(last_slash);
  }
  parse_io_stat_v1(blkio_path + "/blkio.throttle.io_service_bytes", stats);
  
  // Process counts
  stats["cgroup_nprocs"] = read_single_value(base_path + "/cgroup.procs");
  stats["cgroup_nthreads"] = read_single_value(base_path + "/tasks");
  
  // Update monitored values
  for (auto& value : cgroup_stats) {
    if (stats.count(value.first)) {
      value.second.set_value(stats[value.first]);
    }
  }
}

// Parse cgroup v2 cpu.stat file
void cgroupmon::parse_cpu_stat_v2(const std::string& path,
                                  prmon::monitored_value_map& stats) {
  std::ifstream cpu_stat(path);
  if (!cpu_stat.is_open()) {
    return;
  }

  std::string key, value;
  while (cpu_stat >> key >> value) {
    if (key == "usage_usec") {
      stats["cgroup_cpu_total"] = std::stoull(value);
    } else if (key == "user_usec") {
      stats["cgroup_cpu_user"] = std::stoull(value);
    } else if (key == "system_usec") {
      stats["cgroup_cpu_system"] = std::stoull(value);
    } else if (key == "nr_periods") {
      stats["cgroup_cpu_periods"] = std::stoull(value);
    } else if (key == "throttled_usec") {
      stats["cgroup_cpu_throttled"] = std::stoull(value);
    }
  }
}

// Parse cgroup v2 memory.stat file
void cgroupmon::parse_memory_stat_v2(const std::string& path,
                                     prmon::monitored_value_map& stats) {
  std::ifstream mem_stat(path);
  if (!mem_stat.is_open()) {
    return;
  }

  std::string key, value;
  while (mem_stat >> key >> value) {
    unsigned long long val_kb = std::stoull(value) / 1024;
    
    if (key == "anon") {
      stats["cgroup_mem_anon"] = val_kb;
    } else if (key == "file") {
      stats["cgroup_mem_file"] = val_kb;
    } else if (key == "kernel" || key == "kernel_stack") {
      stats["cgroup_mem_kernel"] += val_kb;
    } else if (key == "slab") {
      stats["cgroup_mem_slab"] = val_kb;
    } else if (key == "pgfault") {
      stats["cgroup_mem_pgfault"] = std::stoull(value);
    } else if (key == "pgmajfault") {
      stats["cgroup_mem_pgmajfault"] = std::stoull(value);
    }
  }
}

// Parse cgroup v2 io.stat file
void cgroupmon::parse_io_stat_v2(const std::string& path,
                                 prmon::monitored_value_map& stats) {
  std::ifstream io_stat(path);
  if (!io_stat.is_open()) {
    return;
  }

  std::string line;
  while (std::getline(io_stat, line)) {
    std::istringstream iss(line);
    std::string device, key, value;
    
    iss >> device;  // e.g., "8:0"
    
    while (iss >> key) {
      size_t eq_pos = key.find('=');
      if (eq_pos != std::string::npos) {
        std::string metric = key.substr(0, eq_pos);
        std::string val = key.substr(eq_pos + 1);
        
        if (metric == "rbytes") {
          stats["cgroup_io_read"] += std::stoull(val);
        } else if (metric == "wbytes") {
          stats["cgroup_io_write"] += std::stoull(val);
        }
      }
    }
  }
}

// Parse cgroup v1 cpuacct.stat file
void cgroupmon::parse_cpu_stat_v1(const std::string& path,
                                  prmon::monitored_value_map& stats) {
  std::ifstream cpu_stat(path);
  if (!cpu_stat.is_open()) {
    return;
  }

  std::string key;
  unsigned long long value;
  while (cpu_stat >> key >> value) {
    // cpuacct reports in USER_HZ (typically 100Hz), convert to microseconds
    unsigned long long value_us = value * (1000000 / sysconf(_SC_CLK_TCK));
    
    if (key == "user") {
      stats["cgroup_cpu_user"] = value_us;
    } else if (key == "system") {
      stats["cgroup_cpu_system"] = value_us;
    }
  }
  
  stats["cgroup_cpu_total"] = stats["cgroup_cpu_user"] + stats["cgroup_cpu_system"];
}

// Parse cgroup v1 memory.stat file
void cgroupmon::parse_memory_stat_v1(const std::string& path,
                                     prmon::monitored_value_map& stats) {
  std::ifstream mem_stat(path);
  if (!mem_stat.is_open()) {
    return;
  }

  std::string key;
  unsigned long long value;
  while (mem_stat >> key >> value) {
    unsigned long long val_kb = value / 1024;
    
    if (key == "total_inactive_anon" || key == "total_active_anon") {
      stats["cgroup_mem_anon"] += val_kb;
    } else if (key == "total_inactive_file" || key == "total_active_file") {
      stats["cgroup_mem_file"] += val_kb;
    } else if (key == "total_slab") {
      stats["cgroup_mem_slab"] = val_kb;
    } else if (key == "total_pgfault") {
      stats["cgroup_mem_pgfault"] = value;
    } else if (key == "total_pgmajfault") {
      stats["cgroup_mem_pgmajfault"] = value;
    }
  }
}

// Parse cgroup v1 blkio stats
void cgroupmon::parse_io_stat_v1(const std::string& path,
                                 prmon::monitored_value_map& stats) {
  std::ifstream io_stat(path);
  if (!io_stat.is_open()) {
    return;
  }

  std::string line;
  while (std::getline(io_stat, line)) {
    std::istringstream iss(line);
    std::string device, operation;
    unsigned long long value;
    
    if (iss >> device >> operation >> value) {
      if (operation == "Read") {
        stats["cgroup_io_read"] += value;
      } else if (operation == "Write") {
        stats["cgroup_io_write"] += value;
      }
    }
  }
}

// Helper to read a single value from a file or count lines for proc/thread files
unsigned long long cgroupmon::read_single_value(const std::string& filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    return 0;
  }

  // For cgroup.procs and cgroup.threads, count the number of lines (PIDs)
  if (filepath.find("cgroup.procs") != std::string::npos ||
      filepath.find("cgroup.threads") != std::string::npos) {
    unsigned long long count = 0;
    std::string line;
    while (std::getline(file, line)) {
      if (!line.empty()) {
        count++;
      }
    }
    return count;
  }

  // For single-value files, read the first number
  unsigned long long value;
  file >> value;
  return file.fail() ? 0 : value;
}

// Return current statistics
prmon::monitored_value_map const cgroupmon::get_text_stats() {
  prmon::monitored_value_map cgroup_stat_map{};
  for (const auto& value : cgroup_stats) {
    cgroup_stat_map[value.first] = value.second.get_value();
  }
  return cgroup_stat_map;
}

// Return maximum statistics for JSON
prmon::monitored_value_map const cgroupmon::get_json_total_stats() {
  prmon::monitored_value_map cgroup_stat_map{};
  for (const auto& value : cgroup_stats) {
    cgroup_stat_map[value.first] = value.second.get_max_value();
  }
  return cgroup_stat_map;
}

// Return average statistics for JSON
prmon::monitored_average_map const cgroupmon::get_json_average_stats(
    unsigned long long elapsed_clock_ticks) {
  prmon::monitored_average_map cgroup_stat_map{};
  for (const auto& value : cgroup_stats) {
    cgroup_stat_map[value.first] = value.second.get_average_value();
  }
  return cgroup_stat_map;
}

// Return parameter list
prmon::parameter_list const cgroupmon::get_parameter_list() { return params; }

// Collect hardware information
void const cgroupmon::get_hardware_info(nlohmann::json& hw_json) {
  if (!valid) {
    return;
  }
  
  hw_json["cgroup"]["version"] = 
      (cgroup_version == CgroupVersion::V2 ? "v2" :
       cgroup_version == CgroupVersion::V1 ? "v1" : "hybrid");
  hw_json["cgroup"]["mount_point"] = cgroup_mount_point;
}

// Collect unit information
void const cgroupmon::get_unit_info(nlohmann::json& unit_json) {
  prmon::fill_units(unit_json, params);
}
