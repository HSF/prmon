// Copyright (C) 2020-2025 CERN
// License Apache2 - see LICENCE file

// NVIDIA GPU monitoring class using dynamic loading via dlopen
//
#ifndef PRMON_NVMLMON_H
#define PRMON_NVMLMON_H 1

#include <string>
#include <vector>

#include "Imonitor.h"
#include "MessageBase.h"
#include "parameter.h"
#include "registry.h"

// NVML process utilization sample
typedef struct {
  unsigned int pid;
  unsigned long long timeStamp;
  unsigned int smUtil;
  unsigned int memUtil;
  unsigned int encUtil;
  unsigned int decUtil;
} nvmlProcessUtilizationSample_t;

// NVML process info (v1)
typedef struct {
  unsigned int pid;
  unsigned long long usedGpuMemory;
} nvmlProcessInfo_v1_t;

// NVML process info (v2)
typedef struct {
  unsigned int pid;
  unsigned long long usedGpuMemory;
  unsigned int gpuInstanceId;
  unsigned int computeInstanceId;
} nvmlProcessInfo_v2_t;

// v3 has the same layout as v2 per the official NVML header
typedef nvmlProcessInfo_v2_t nvmlProcessInfo_v3_t;

// Forward declaration for NVML device handle (defined as opaque pointer)
typedef struct nvmlDevice_st* nvmlDevice_t;

class nvmlmon final : public Imonitor, public MessageBase {
 private:
  // Setup the parameters to monitor here
  const prmon::parameter_list params = {{"ngpus", "1", "1"},
                                        {"gpusmpct", "%", "%"},
                                        {"gpumempct", "%", "%"},
                                        {"gpufbmem", "kB", "kB"}};

  // Map of classes that represent each monitored quantity
  // Will be initialised from the above parameter list
  prmon::monitored_list nvml_stats;

  // Handle to the dynamically loaded libnvidia-ml.so
  void* nvml_handle;

  // Set to true when NVML is successfully loaded and initialized
  bool valid;

  // Number of GPUs detected on the system
  unsigned int ngpus{};

  bool load_nvml_lib();

  bool init_nvml();

  // Query utilization and memory samples for a single GPU device
  bool query_device_samples(nvmlDevice_t device, unsigned int gpu_idx,
                            unsigned int& util_count, unsigned int& mem_count,
                            unsigned long long& current_max_timestamp);

  // Match monitored PIDs against sampled data, accumulating into stats
  bool accumulate_process_stats(const std::vector<pid_t>& pids,
                                unsigned int util_count,
                                unsigned int mem_count,
                                prmon::monitored_value_map& stats);

  // Vectors to store utilization and memory info
  std::vector<nvmlProcessUtilizationSample_t> utilization;
  std::vector<nvmlProcessInfo_v3_t> memory_info;

  const unsigned int max_samples = 100;

  const unsigned long long BYTES_PER_KB = 1024;

  unsigned long long last_seen_timestamp{};

 public:
  nvmlmon();
  ~nvmlmon();

  void update_stats(const std::vector<pid_t>& pids,
                    const std::string read_path = "");

  prmon::monitored_value_map const get_text_stats();
  prmon::monitored_value_map const get_json_total_stats();
  prmon::monitored_average_map const get_json_average_stats(
      unsigned long long elapsed_clock_ticks);
  prmon::parameter_list const get_parameter_list();

  void const get_hardware_info(nlohmann::json& hw_json);
  void const get_unit_info(nlohmann::json& unit_json);
  bool const is_valid() { return valid; }
};
REGISTER_MONITOR(Imonitor, nvmlmon, "Monitor NVIDIA GPU activity via NVML")

#endif  // PRMON_NVMLMON_H
