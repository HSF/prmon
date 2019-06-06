// Copyright (C) CERN, 2019

// A few helper functions to get CPU information

#include <fstream>
#include <iostream>
#include <string>

#include "cpuinfo.h"

// Method to get the total number of processors
int cpuinfo::get_number_of_cpus() {
  int nCPUs = 0, maxId = -1;

  std::ifstream cpuInfoFile{"/proc/cpuinfo"};
  if(!cpuInfoFile.is_open()) {
    std::cerr << "Failed to open /proc/cpuinfo" << std::endl;
    return -1;
  }

  const std::string key = "processor";
  std::string line;
  while(std::getline(cpuInfoFile,line)) {
    if (line.empty()) continue;
    size_t splitIdx = line.find(":");
    std::string val;
    if (splitIdx != std::string::npos) val = line.substr(splitIdx + 1);
    if (line.size() >= key.size() && line.compare(0, key.size(), key) == 0) {
      nCPUs++;
      if(!val.empty()) {
        int curId = std::stoi(val);
        maxId = std::max(curId, maxId); 
      } // end of getting maxId
    } // end of incrementing nCPUs 
  } // end of reading cpuInfoFile  

  cpuInfoFile.close();

  // Consistency check
  if(nCPUs != (maxId + 1)) {
    std::cout << "CPU ID assignment in /proc/cpuinfo seems fishy!" << std::endl;
  }

  return nCPUs;
}

// Method to get the processor clock freqs
std::vector<float> cpuinfo::get_processor_clock_freqs() {
  std::vector<float> result;

  std::ifstream cpuInfoFile{"/proc/cpuinfo"};
  if(!cpuInfoFile.is_open()) {
    std::cerr << "Failed to open /proc/cpuinfo" << std::endl;
    return result;
  }

  const std::string key = "cpu MHz";
  std::string line;
  while(std::getline(cpuInfoFile,line)) {
    if (line.empty()) continue;
    size_t splitIdx = line.find(":");
    std::string val;
    if (splitIdx != std::string::npos) val = line.substr(splitIdx + 1);
    if (line.size() >= key.size() && line.compare(0, key.size(), key) == 0) {
       result.push_back(std::stof(val));
    } // end of filling clock freqs - order doesn't change
  } // end of reading cpuInfoFile  

  cpuInfoFile.close();

  return result;
}

// Method to get the scaling information
// 0 for false, 1 for true, 2 for error
unsigned int cpuinfo::cpu_scaling_info(int nCPUs) {
  unsigned int result = 0;

  for(int cpu = 0; cpu < nCPUs; ++cpu) {
    std::string fileName = "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cpufreq/scaling_governor";

    std::ifstream governorFile{fileName};
    if(!governorFile.is_open()) {
      std::cerr << "Failed to open " << fileName << std::endl;
      return 2;
    }

    // The governer file has a single line
    std::string line; 
    if(std::getline(governorFile,line) && line != "performance") { result = 1; break; }

    governorFile.close();
  } // end of looping over cpus

  return result;
}
