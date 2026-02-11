// Copyright (C) 2020-2025 CERN
//
// NVIDIA GPU monitoring class
//

#ifndef PRMON_NVIDIAMON_H 
#define PRMON_NVIDIAMON_H 1

#include <map>
#include <string>
#include <vector>
#include <nvml.h>

#include "Imonitor.h"
#include "MessageBase.h"
#include "parameter.h"
#include "registry.h"


class nvidiamon final : public Imonitor, public MessageBase { 
 private:
  // const static std::vector<std::string> default_nvidia_params{
  //   "ngpus", "gpusmpct", "gpumempct", "gpufbmem"};
  const prmon::parameter_list params = {{"ngpus", "1", "1"},
                                        {"gpusmpct", "%", "%"},
                                        {"gpumempct", "%", "%"},
                                        {"gpufbmem", "kB", "kB"}};
  
  // Map of classes that represent each monitored quantity
  // Will be initialised from the above parameter list
  prmon::monitored_list nvidia_stats;

  // Set a boolean to see if we have a valid nvidia setup
  bool valid;

  // Count GPUs on the system
  static unsigned int ngpus;
  
  // Test if nvml is available
  bool init_nvml();
  
  // Vectors to store utilization and memory info
  std::vector<nvmlProcessUtilizationSample_t> utilization;
  std::vector<nvmlProcessInfo_t> memory_info;

  // Max number of samples to take
  const unsigned int max_samples = 100;

  // Conversion from  B to kB for (this is to be more consistent with other
  // memory units in prmon)
  const unsigned int B_to_KB = 1024; 

  // Last seen timestamp
  unsigned long long last_seen_timestamp = 0;

 public:
  nvidiamon();
  ~nvidiamon();

  void update_stats(const std::vector<pid_t>& pids,
    const std::string read_path = "");
    
  // These are the stat getter methods which retrieve current statistics
  prmon::monitored_value_map const get_text_stats();
  prmon::monitored_value_map const get_json_total_stats();
  prmon::monitored_average_map const get_json_average_stats(
        unsigned long long elapsed_clock_ticks);
  prmon::parameter_list const get_parameter_list();

  void const get_hardware_info(nlohmann::json& hw_json);
  void const get_unit_info(nlohmann::json& unit_json);
  bool const is_valid() { return valid; } 

};
REGISTER_MONITOR(Imonitor, nvidiamon, "Monitor NVIDIA GPU activity")


#endif // PRMON_NVIDIAMON_H