// Copyright (C) 2018-2021 CERN
// License Apache2 - see LICENCE file

#include "netmon.h"

#include <dirent.h>
#include <time.h>

#include <cstring>
#include <memory>
#include <sstream>

#include "utils.h"

#define MONITOR_NAME "netmon"

// Constructor; uses RAII pattern to open all monitored
// network device streams and to take initial values
// to the monitor relative differences
netmon::netmon(std::vector<std::string> netdevs)
    : interface_params{}, network_if_streams{} {
  log_init(MONITOR_NAME);
#undef MONITOR_NAME
  interface_params.reserve(params.size());
  for (const auto& param : params) interface_params.push_back(param.get_name());

  if (netdevs.size() == 0) {
    monitored_netdevs = get_all_network_devs();
  } else {
    monitored_netdevs = netdevs;
  }
  open_interface_streams();

  // Ensure internal stat counters are initialised properly
  read_raw_network_stats(network_stats_initial);
  for (const auto& param : params) {
    net_stats.emplace(
        param.get_name(),
        prmon::monitored_value(param, true,
                               network_stats_initial[param.get_name()]));
  }
}

// Get all available network devices
// This is all a bit yuk, using C style directory
// parsing. From C++17 we should use the filesystem
// library, but only when we decide it's reasonable
// to no longer support older compilers.
std::vector<std::string> const netmon::get_all_network_devs() {
  std::vector<std::string> devices{};
  DIR* d;
  struct dirent* dir;
  const char* netdir = "/sys/class/net";
  d = opendir(netdir);
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (!(!std::strcmp(dir->d_name, ".") || !std::strcmp(dir->d_name, "..")))
        devices.push_back(dir->d_name);
    }
    closedir(d);
  } else {
    error("Failed to open " + std::string(netdir) +
          " to get list of network devices. " +
          "No network data will be available");
  }
  return devices;
}

// Opens an ifstream for all monitored network parameters
// Stored as unique_ptrs to ensure they are closed when
// the instance is destroyed
void netmon::open_interface_streams() {
  for (const auto& if_param : interface_params) {
    for (const auto& device : monitored_netdevs) {
      // make_unique would be better, but mandates C++14
      std::unique_ptr<std::ifstream> u_strm_ptr =
          std::unique_ptr<std::ifstream>(
              new std::ifstream(get_sys_filename(device, if_param)));
      network_if_streams[if_param][device] = std::move(u_strm_ptr);
    }
  }
}

// Read raw stat values
void netmon::read_raw_network_stats(prmon::monitored_value_map& stats) {
  for (const auto& if_param : interface_params) {
    prmon::mon_value value_read{};
    stats[if_param] = 0;
    for (const auto& device : monitored_netdevs) {
      network_if_streams[if_param][device]->seekg(0);
      *network_if_streams[if_param][device] >> value_read;
      stats[if_param] += value_read;
    }
  }
}
void netmon::read_raw_network_stats_test(
    std::map<std::string, unsigned long long>& stats,
    const std::string& read_path) {
  static int iteration = 0;
  ++iteration;
  if (iteration == 1) {
    // Reset all net_stats initialised in constructor to 0 for testing
    net_stats.clear();
    for (const auto& param : params) {
      net_stats.emplace(param.get_name(),
                        prmon::monitored_value(param, true, 0));
    }
  }
  for (const auto& if_param : interface_params) {
    unsigned long long value_read{};
    stats[if_param] = 0;
    std::string net_fname = read_path + if_param;
    std::ifstream net_stream(net_fname);
    net_stream >> value_read;
    stats[if_param] += value_read;
  }
}

// Update statistics
void netmon::update_stats(const std::vector<pid_t>& pids,
                          const std::string read_path) {
  prmon::monitored_value_map network_stats_update;

  if (read_path.size()) {
    read_raw_network_stats_test(network_stats_update, read_path);
  } else {
    read_raw_network_stats(network_stats_update);
  }
  for (auto& value : net_stats) {
    value.second.set_value(network_stats_update[value.first]);
  }
}

// Relative counter statistics for text file
prmon::monitored_value_map const netmon::get_text_stats() {
  prmon::monitored_value_map net_stat_map{};
  for (const auto& value : net_stats) {
    net_stat_map[value.first] = value.second.get_value();
  }
  return net_stat_map;
}

// Also relative counters for JSON totals
prmon::monitored_value_map const netmon::get_json_total_stats() {
  return get_text_stats();
}

// For JSON averages, divide by elapsed time
prmon::monitored_average_map const netmon::get_json_average_stats(
    unsigned long long elapsed_clock_ticks) {
  prmon::monitored_average_map json_average_stats{};
  for (const auto& value : net_stats) {
    json_average_stats[value.first] =
        prmon::avg_value(value.second.get_value() * sysconf(_SC_CLK_TCK)) /
        elapsed_clock_ticks;
  }
  return json_average_stats;
}

// Return the parameter list
prmon::parameter_list const netmon::get_parameter_list() { return params; }

// Collect related hardware information
void const netmon::get_hardware_info(nlohmann::json& hw_json) { return; }

void const netmon::get_unit_info(nlohmann::json& unit_json) {
  prmon::fill_units(unit_json, params);
  return;
}
