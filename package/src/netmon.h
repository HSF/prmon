// Copyright (C) CERN, 2018
//
// Network monitoring class
//
// This is a more efficient network monitor that holds
// its istreams in a structure, so that they don't have
// to be opened and closed on each monitoring cycle.

#ifndef PRMON_NETMON_H
#define PRMON_NETMON_H 1

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>

#include "Imonitor.h"
#include "registry.h"

class netmon final : public Imonitor {
 private:
  // Which network interface paramters to measure (in this simple case
  // these are also the output key names)
  std::vector<std::string> interface_params;

  // Which network interfaces to monitor
  std::vector<std::string> monitored_netdevs;

  // Nested dictionary of network_if_streams[PARMETER][DEVICE][ISTREAM*]
  std::map<
      std::string,
      std::unordered_map<std::string, std::unique_ptr<std::ifstream>>>
      network_if_streams;

  // Container for stats, initial and current
  std::map<std::string, unsigned long long> network_stats_start,
      network_stats;

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

  void update_stats(const std::vector<pid_t>& pids) {
    read_raw_network_stats(network_stats);
  }

  // These are the stat getter methods which retrieve current statistics
  std::map<std::string, unsigned long long> const get_text_stats();
  std::map<std::string, unsigned long long> const get_json_total_stats();
  std::map<std::string, double> const get_json_average_stats(unsigned long long elapsed_clock_ticks);

  // This is the hardware information getter that runs once
  void const get_hardware_info(nlohmann::json& hw_json);

};
REGISTER_MONITOR(Imonitor, netmon, "Monitor network activity (device level)")

#endif  // PRMON_NETMON_H
