// Copyright (C) 2018-2025 CERN
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
#include "MessageBase.h"
#include "parameter.h"
#include "registry.h"

class netmon final : public Imonitor, public MessageBase {
 private:
  // Setup the parameters to monitor here
  const prmon::parameter_list params = {{"rx_bytes", "B", "B/s"},
                                        {"rx_packets", "1", "1/s"},
                                        {"tx_bytes", "B", "B/s"},
                                        {"tx_packets", "1", "1/s"}};

  // Dynamic monitoring container for value measurements
  // This will be filled at initialisation, taking the names
  // from the above params
  prmon::monitored_list net_stats;

  // Initial values, as these stats are global counters
  prmon::monitored_value_map network_stats_initial;

  // Vector for network interface parameters to measure (will be constructed)
  std::vector<std::string> interface_params;

  // Which network interfaces to monitor
  std::vector<std::string> monitored_netdevs;

  // Nested dictionary of network_if_streams[PARAMETER][DEVICE][ISTREAM*]
  std::map<std::string,
           std::unordered_map<std::string, std::unique_ptr<std::ifstream>>>
      network_if_streams;

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
  void read_raw_network_stats(prmon::monitored_value_map& values);

  void read_raw_network_stats_test(prmon::monitored_value_map& values,
                                   const std::string& read_path);

 public:
  netmon(std::vector<std::string> netdevs);
  netmon() : netmon(std::vector<std::string>{}){};

  void update_stats(const std::vector<pid_t>& pids,
                    const std::string read_path = "");

  // These are the stat getter methods which retrieve current statistics
  prmon::monitored_value_map const get_text_stats();
  prmon::monitored_value_map const get_json_total_stats();
  prmon::monitored_average_map const get_json_average_stats(
      unsigned long long elapsed_clock_ticks);
  prmon::parameter_list const get_parameter_list();

  // This is the hardware information getter that runs once
  void const get_hardware_info(nlohmann::json& hw_json);
  void const get_unit_info(nlohmann::json& unit_json);
  bool const is_valid() { return true; }
};
REGISTER_MONITOR_1ARG(Imonitor, netmon,
                      "Monitor network activity (device level)",
                      std::vector<std::string>)

#endif  // PRMON_NETMON_H
