// Copyright (C) CERN, 2020
//
// Process and thread number monitoring class
//

#ifndef PRMON_NVIDIAMON_H
#define PRMON_NVIDIAMON_H 1

#include <map>
#include <string>
#include <vector>

#include "Imonitor.h"
#include "parameter.h"
#include "registry.h"

class nvidiamon final : public Imonitor {
 private:
  // const static std::vector<std::string> default_nvidia_params{
  //   "ngpus", "gpusmpct", "gpumempct", "gpufbmem"};
  const prmon::parameter_list params = {{"ngpus", "1", "1", false},
                                        {"gpusmpct", "%", "%", false},
                                        {"gpumempct", "%", "%", false},
                                        {"gpufbmem", "kB", "kB", false}};

  // Which paramters to measure and output key names
  std::vector<std::string> nvidia_params;

  // Container for total stats
  std::map<std::string, unsigned long long> nvidia_stats;
  std::map<std::string, unsigned long long> nvidia_peak_stats;
  std::map<std::string, double> nvidia_average_stats;
  std::map<std::string, unsigned long long> nvidia_total_stats;

  // Counter for number of iterations
  unsigned long iterations;

  // Set a boolean to see if we have a valid nvidia setup
  bool valid;

  // Count GPUs on the system
  unsigned int ngpus;

  // Test if nvidia-smi is available
  bool test_nvidia_smi();

  // Conversion from MB to kB for (this is to be more consistent with other
  // memory units in prmon)
  const unsigned int MB_to_KB = 1024;

 public:
  nvidiamon();

  void update_stats(const std::vector<pid_t>& pids);

  // These are the stat getter methods which retrieve current statistics
  std::map<std::string, unsigned long long> const get_text_stats();
  std::map<std::string, unsigned long long> const get_json_total_stats();
  std::map<std::string, double> const get_json_average_stats(
      unsigned long long elapsed_clock_ticks);

  void const get_hardware_info(nlohmann::json& hw_json);
  void const get_unit_info(nlohmann::json& unit_json);
  bool const is_valid() { return valid; }
};
REGISTER_MONITOR(Imonitor, nvidiamon, "Monitor NVIDIA GPU activity")

#endif  // PRMON_NVIDIAMON_H
