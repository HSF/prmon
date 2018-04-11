// Copyright (C) CERN, 2018

#include "netmon.h"

#include <cstring>
#include <dirent.h>
#include <memory>

const static std::vector<std::string> default_if_params{"rx_bytes", "rx_packets", "tx_bytes", "tx_packets"};

netmon::netmon():
interface_params{default_if_params},
monitored_netdevs{},
network_if_streams{}
{
  // Default constructor will monitor all devices
  get_network_devs();

  // Open all istreams we need for monitoring
  open_interface_streams();
}


netmon::netmon(std::vector<std::string> netdevs):
interface_params{default_if_params},
monitored_netdevs{},
network_if_streams{}
{
  monitored_netdevs = netdevs;
  open_interface_streams();
}


// Get all available network devices
// This is all a bit yuk, using C style directory
// parsing. From C++17 we should use the filesystem
// library, but only when we decide it's reasonable
// to no longer support older compilers.
void netmon::get_network_devs() {
  DIR* d;
  struct dirent* dir;
  const char* netdir = "/sys/class/net";
  d = opendir(netdir);
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (!(!std::strcmp(dir->d_name, ".") || !std::strcmp(dir->d_name, "..")))
        monitored_netdevs.push_back(dir->d_name);
    }
    closedir(d);
  } else {
    std::cerr << "Failed to open " << netdir
              << " to get list of network devices. "
              << "No network data will be available" << std::endl;
  }
}

void netmon::open_interface_streams() {
  for (const auto& if_param: interface_params) {
    for (const auto& device: monitored_netdevs) {
      // make_unique is better, but mandates C++14
      //std::unique_ptr<std::istream> u_strm_ptr = std::make_unique<std::ifstream>(filename);
      std::unique_ptr<std::istream> u_strm_ptr = std::unique_ptr<std::ifstream>(new std::ifstream(get_sys_filename(device, if_param)));
      network_if_streams[if_param][device] = std::move(u_strm_ptr);
    }
  }
}


std::unordered_map<std::string, unsigned long long> netmon::read_network_stats() {
  std::unordered_map<std::string, unsigned long long> stats{};
  read_network_stats(stats);
  return stats;
}

void netmon::read_network_stats(std::unordered_map<std::string, unsigned long long>& stats) {
  for (const auto& if_param: interface_params) {
    unsigned long long value_read{};
    stats[if_param] = 0;
    for (const auto& device: monitored_netdevs) {
      network_if_streams[if_param][device]->seekg(0);
      *network_if_streams[if_param][device] >> value_read;
      stats[if_param] += value_read;
    }
  }
}

