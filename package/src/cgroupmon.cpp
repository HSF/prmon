// Copyright (C) 2018-2026 CERN
// License Apache2 - see LICENCE file

#include "cgroupmon.h"

#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "utils.h"

#define MONITOR_NAME "cgroupmon"

// Constructor - detect cgroup version and setup monitoring
cgroupmon::cgroupmon()
    : valid{false}, baseline_taken{false}, cgroup_path{}, cgroup_mount_point{} {
  log_init(MONITOR_NAME);
#undef MONITOR_NAME

  // Initialize monitored values
  for (const auto& param : params) {
    const bool cumulative = cumulative_params.count(param.get_name()) > 0;
    cgroup_stats.emplace(param.get_name(),
                         prmon::monitored_value(param, cumulative));
  }

  // Find cgroup mount point
  cgroup_mount_point = find_cgroup_mount_point();

  // Only a unified (v2) hierarchy is supported, which is identified by the
  // presence of cgroup.controllers at the mount point
  struct stat st;
  if (stat((cgroup_mount_point + "/cgroup.controllers").c_str(), &st) != 0) {
    info("No cgroup v2 hierarchy found at " + cgroup_mount_point +
         ", cgroupmon will be inactive");
    return;
  }

  valid = true;
  info("Cgroup v2 monitoring enabled");
}

// Find the cgroup v2 path (relative to the mount point) for a given PID,
// returning an empty string if it cannot be determined (e.g., the process
// has exited, or there is no unified hierarchy entry)
std::string cgroupmon::find_cgroup_path(pid_t pid,
                                        const std::string& read_path) {
  std::stringstream cgroup_file;
  cgroup_file << read_path << "/proc/" << pid << "/cgroup";

  std::ifstream cgroup_stream(cgroup_file.str());
  if (!cgroup_stream.is_open()) {
    return "";
  }

  std::string line;
  // For cgroup v2, look for the line starting with "0::"
  while (std::getline(cgroup_stream, line)) {
    if (line.compare(0, 3, "0::") == 0) {
      std::string path = line.substr(3);  // Remove "0::" prefix
      // Only accept absolute paths that stay inside the mount point
      if (path.empty() || path[0] != '/' ||
          path.find("/..") != std::string::npos) {
        return "";
      }
      return path;
    }
  }

  return "";
}

// Find the cgroup filesystem mount point
std::string cgroupmon::find_cgroup_mount_point() { return "/sys/fs/cgroup"; }

// Update statistics from cgroup
void cgroupmon::update_stats(const std::vector<pid_t>& pids,
                             const std::string read_path) {
  // N.B. validity is not checked here: prmon only polls valid monitors, and
  // this allows precooked tests (via read_path) to run on any host
  if (pids.empty()) {
    return;
  }

  // Use first PID to determine cgroup path
  // (all processes in the tree should be in the same cgroup)
  cgroup_path = find_cgroup_path(pids[0], read_path);
  if (cgroup_path.empty()) {
    debug("Could not determine cgroup of pid " + std::to_string(pids[0]));
    return;
  }

  // Construct full path to cgroup directory
  std::string full_cgroup_path = read_path + cgroup_mount_point + cgroup_path;

  try {
    read_cgroup_v2_stats(full_cgroup_path);
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
  stats["cgroup_mem_current"] =
      read_single_value(cgroup_path + "/memory.current") / 1024;
  stats["cgroup_mem_max"] = read_memory_max(cgroup_path + "/memory.max") / 1024;

  // I/O statistics
  parse_io_stat_v2(cgroup_path + "/io.stat", stats);

  // Process and thread counts
  stats["cgroup_nprocs"] = read_single_value(cgroup_path + "/cgroup.procs");
  stats["cgroup_nthreads"] = read_single_value(cgroup_path + "/cgroup.threads");

  // Update monitored values. Cumulative counters include all usage since
  // the cgroup was created, so the first reading is taken as their offset
  // and they then report usage since prmon started. (A counter that cannot
  // be read at the first update gets no offset.)
  for (auto& value : cgroup_stats) {
    const auto reading = stats.find(value.first);
    if (reading == stats.end()) continue;
    if (!baseline_taken && cumulative_params.count(value.first)) {
      value.second.set_offset(reading->second);
    }
    value.second.set_value(reading->second);
  }
  baseline_taken = true;
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

  // "kernel" (Linux >= 5.18) already includes "kernel_stack", so only use
  // "kernel_stack" on older kernels that lack "kernel"
  bool have_kernel = false, have_kernel_stack = false;
  unsigned long long kernel_stack_kb = 0;

  std::string key, value;
  while (mem_stat >> key >> value) {
    unsigned long long val_kb = std::stoull(value) / 1024;

    if (key == "anon") {
      stats["cgroup_mem_anon"] = val_kb;
    } else if (key == "file") {
      stats["cgroup_mem_file"] = val_kb;
    } else if (key == "kernel") {
      stats["cgroup_mem_kernel"] = val_kb;
      have_kernel = true;
    } else if (key == "kernel_stack") {
      kernel_stack_kb = val_kb;
      have_kernel_stack = true;
    }
  }
  if (!have_kernel && have_kernel_stack) {
    stats["cgroup_mem_kernel"] = kernel_stack_kb;
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

// Helper to read a single value from a file or count lines for proc/thread
// files
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

// Read memory.max, returning 0 when there is no limit (the file then
// contains "max") or the value cannot be read
unsigned long long cgroupmon::read_memory_max(const std::string& filepath) {
  std::ifstream file(filepath);
  std::string token;
  if (!(file >> token) || token == "max") {
    return 0;
  }
  try {
    return std::stoull(token);
  } catch (const std::exception&) {
    return 0;
  }
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
    const std::string avg_unit = value.second.get_avg_unit();
    // An empty average unit means no meaningful average exists, so the
    // value must be omitted (fill_units() likewise omits its unit)
    if (avg_unit.empty()) continue;
    if (avg_unit.size() > 2 &&
        avg_unit.compare(avg_unit.size() - 2, 2, "/s") == 0) {
      // Rate units: cumulative counter divided by elapsed time (as iomon)
      cgroup_stat_map[value.first] =
          elapsed_clock_ticks ? prmon::avg_value(value.second.get_value()) *
                                    sysconf(_SC_CLK_TCK) / elapsed_clock_ticks
                              : 0.0;
    } else {
      cgroup_stat_map[value.first] = value.second.get_average_value();
    }
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

  hw_json["HW"]["cgroup"]["version"] = "v2";
}

// Collect unit information
void const cgroupmon::get_unit_info(nlohmann::json& unit_json) {
  prmon::fill_units(unit_json, params);
}
