// Copyright (C) 2020-2026 CERN
//
// NVIDIA GPU monitoring class
//
#ifndef PRMON_NVIDIAMON_H
#define PRMON_NVIDIAMON_H 1

#include <map>
#include <string>
#include <vector>

#include "Imonitor.h"
#include "MessageBase.h"
#include "parameter.h"
#include "registry.h"

// The following constants, types, and function pointer declarations
// mirror the Nvidia NVML API and are resolved at runtime via dlopen
// to avoid a link-time dependency on the NVML library.

// NVML process utilization sample
typedef struct {
  unsigned int pid;
  unsigned long long timeStamp;
  unsigned int smUtil;
  unsigned int memUtil;
  unsigned int encUtil;
  unsigned int decUtil;
} nvmlProcessUtilizationSample_t;

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

class nvidiamon final : public Imonitor, public MessageBase {
 private:
  // Enum to decide which method to use to monitor nvidia GPUs
  enum class MonitorMethod { NVML, SMI, NONE };

  // const static std::vector<std::string> default_nvidia_params{
  //   "ngpus", "gpusmpct", "gpumempct", "gpufbmem"};
  const prmon::parameter_list params = {{"ngpus", "1", "1"},
                                        {"gpusmpct", "%", "%"},
                                        {"gpumempct", "%", "%"},
                                        {"gpufbmem", "kB", "kB"}};

  // Map of classes that represent each monitored quantity
  // Will be initialised from the above parameter list
  prmon::monitored_list nvidia_stats;

  // Handle to the dynamically loaded libnvidia-ml.so
  void* nvml_handle{nullptr};

  // Set a boolean to see if we have a valid nvidia setup
  bool valid;

  // Set a enum to see which method is active
  MonitorMethod active_method{MonitorMethod::NONE};

  // Count GPUs on the system
  unsigned int ngpus{};

  // Load the NVML library via dlopen
  bool load_nvml_lib();

  // Initialize NVML
  bool init_nvml();

  // Test if nvidia-smi is available
  bool test_nvidia_smi();

  // Query utilization and memory samples for a single GPU device
  bool query_device_samples(nvmlDevice_t device, unsigned int gpu_idx,
                            unsigned int& util_count, unsigned int& mem_count,
                            unsigned long long& current_max_timestamp);

  // Match monitored PIDs against sampled data, accumulating into stats
  bool accumulate_process_stats(const std::vector<pid_t>& pids,
                                unsigned int util_count, unsigned int mem_count,
                                prmon::monitored_value_map& stats);

  // NVML methods
  void update_stats_nvml(const std::vector<pid_t>& pids,
                         const std::string read_path);
  void const get_hardware_info_nvml(nlohmann::json& hw_json);

  // SMI methods
  void update_stats_smi(const std::vector<pid_t>& pids,
                        const std::string read_path);
  void const get_hardware_info_smi(nlohmann::json& hw_json);

  // Vectors to store utilization and memory info
  std::vector<nvmlProcessUtilizationSample_t> utilization;
  std::vector<nvmlProcessInfo_v3_t> memory_info;

  const unsigned int max_samples = 100;

  // Conversion from MB to kB for (this is to be more consistent with other
  // memory units in prmon)
  const unsigned int MB_to_KB = 1024;
  const unsigned long long BYTES_PER_KB = 1024;
  unsigned long long last_seen_timestamp{};

 public:
  nvidiamon();
  ~nvidiamon();

  void update_stats(const std::vector<pid_t>& pids,
                    const std::string read_path = "");

  // Alternative to cmd_pipe_output() for testing
  std::pair<int, std::vector<std::string>> read_gpu_stats_test(
      const std::string read_path);

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

#endif  // PRMON_NVIDIAMON_H
