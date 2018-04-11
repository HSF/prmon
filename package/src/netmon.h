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
#include <unordered_map>
#include <vector>

class netmon {
private:
  // Which network interface paramters to measure
  const std::vector<std::string> interface_params;

  // Which network interfaces to monitor
  std::vector<std::string> monitored_netdevs;

  // Nested dictionary of network_if_streams[PARMETER][DEVICE][ISTREAM*]
  std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<std::istream>>> network_if_streams;

  // Find all network interfaces on the system
  void get_network_devs();

  // Open streams for each of the network interfaces and parameters
  void open_interface_streams();

  // Helper to map a device and parameter to a /sys filename
  inline std::string get_sys_filename(std::string device, std::string param) {
    std::string filename = "/sys/class/net/" + device + "/statistics/" + param;
    return filename;
  }


public:
  netmon();
  netmon(std::vector<std::string> netdevs);
  ~netmon();

  const std::vector<std::string> get_interface_paramter_names() {
    return interface_params;
  }

  const std::vector<std::string> get_interface_names() {
    return monitored_netdevs;
  }

  // Return network stats in a variable
  std::unordered_map<std::string, unsigned long long> read_network_stats();

  // Return network stats by modifying a reference
  void read_network_stats(std::unordered_map<std::string, unsigned long long>& values);

};

#endif // PRMON_NETMON_H
