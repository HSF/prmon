// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

// Network monitoring class
//
// This is a more efficient network monitor that holds
// its istreams in a structure, so that they don't have
// to be opened and closed on each monitoring cycle.

#ifndef PRMON_NETMON_H
#define PRMON_NETMON_H 1

#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Imonitor.h"
#include "parameter.h"
#include "registry.h"

class netmon final : public Imonitor {
 private:
  // Setup the parameters to monitor here
  const prmon::parameter_list params = {{"rx_bytes", "B", "B/s", true},
                                        {"rx_packets", "1", "1/s", true},
                                        {"tx_bytes", "B", "B/s", true},
                                        {"tx_packets", "1", "1/s", true}};

  // Vector for network interface paramters to measure (will be constructed)
  std::vector<std::string> interface_params;

  // Which network interfaces to monitor
  std::vector<std::string> monitored_netdevs;

  // Nested dictionary of network_if_streams[PARMETER][DEVICE][ISTREAM*]
  std::map<std::string,
           std::unordered_map<std::string, std::unique_ptr<std::ifstream>>>
      network_if_streams;

  // Container for stats, initial and current
  std::map<std::string, unsigned long long> network_stats, network_stats_last,
      network_net_counters;

  // Find all network interfaces on the system
  std::vector<std::string> const get_all_network_devs();

  // Open streams for each of the network interfaces and parameters
  void open_interface_streams();

  // Helper to map a device and parameter to a /sys filename
  inline std::string const get_sys_filename(std::string device,
                                            std::string param) {
    std::string filename = "/sys/class/net/" + device + "/statistics/" + param;
    return filename;
  }

  // Internal method to read "raw" network stats
  void read_raw_network_stats(
      std::map<std::string, unsigned long long>& values);

 public:
  netmon(std::vector<std::string> netdevs);
  netmon() : netmon(std::vector<std::string>{}){};

  void update_stats(const std::vector<pid_t>& pids);

  // These are the stat getter methods which retrieve current statistics
  std::map<std::string, unsigned long long> const get_text_stats();
  std::map<std::string, unsigned long long> const get_json_total_stats();
  std::map<std::string, double> const get_json_average_stats(
      unsigned long long elapsed_clock_ticks);

  // This is the hardware information getter that runs once
  void const get_hardware_info(nlohmann::json& hw_json);
  void const get_unit_info(nlohmann::json& unit_json);
  bool const is_valid() { return true; }
};
REGISTER_MONITOR(Imonitor, netmon, "Monitor network activity (device level)")

#endif  // PRMON_NETMON_H
