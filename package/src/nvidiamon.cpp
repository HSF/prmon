// Copyright (C) 2020-2026 CERN
// License Apache2 - see LICENCE file

#include "nvidiamon.h"

#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "utils.h"

#define MONITOR_NAME "nvidiamon"

// The following constants, types, and function pointer declarations
// mirror the Nvidia NVML API and are resolved at runtime via dlopen
// to avoid a link-time dependency on the NVML library.

// NVML return code constants
#define NVML_SUCCESS 0
#define NVML_ERROR_NOT_FOUND 6
#define NVML_ERROR_INSUFFICIENT_SIZE 7

// NVML device name buffer size
#define NVML_DEVICE_NAME_BUFFER_SIZE 128

// NVML return type
typedef int nvmlReturn_t;

// NVML memory info struct
typedef struct {
  unsigned long long total;  // Total physical device memory (in bytes)
  unsigned long long free;   // Unallocated device memory (in bytes)
  unsigned long long used;   // Allocated device memory (in bytes)
} nvmlMemory_t;

// NVML clock types
typedef enum {
  NVML_CLOCK_GRAPHICS = 0,
  NVML_CLOCK_SM = 1,
  NVML_CLOCK_MEM = 2,
  NVML_CLOCK_VIDEO = 3,
} nvmlClockType_t;

// NVML function pointers
// Lifecycle
static nvmlReturn_t (*nvmlInit)(void);
static nvmlReturn_t (*nvmlShutdown)(void);
static const char* (*nvmlErrorString)(nvmlReturn_t);

// Device
static nvmlReturn_t (*nvmlDeviceGetCount)(unsigned int* deviceCount);
static nvmlReturn_t (*nvmlDeviceGetHandleByIndex)(unsigned int index,
                                                  nvmlDevice_t* device);

// Process
static nvmlReturn_t (*nvmlDeviceGetProcessUtilization)(
    nvmlDevice_t device, nvmlProcessUtilizationSample_t* utilization,
    unsigned int* processSamplesCount, unsigned long long lastSeenTimeStamp);

// Process — highest available version (v3 > v2 > v1) is resolved at load time.
// Function pointer uses nvmlProcessInfo_v3_t* (largest layout); v1/v2 structs
// are a strict prefix so casting up is safe — only pid and usedGpuMemory are
// read.
static nvmlReturn_t (*nvmlDeviceGetComputeRunningProcesses)(
    nvmlDevice_t device, unsigned int* infoCount, nvmlProcessInfo_v3_t* infos);

// Device info
static nvmlReturn_t (*nvmlDeviceGetName)(nvmlDevice_t device, char* name,
                                         unsigned int length);
static nvmlReturn_t (*nvmlDeviceGetMaxClockInfo)(nvmlDevice_t device,
                                                 nvmlClockType_t type,
                                                 unsigned int* clock);
static nvmlReturn_t (*nvmlDeviceGetMemoryInfo)(nvmlDevice_t device,
                                               nvmlMemory_t* memory);

// Constructor; uses RAII pattern to be valid after construction
nvidiamon::nvidiamon() {
  log_init(MONITOR_NAME);
#undef MONITOR_NAME
  for (const auto& param : params) {
    nvidia_stats.emplace(param.get_name(), prmon::monitored_value(param));
  }

  valid = true;

  if (load_nvml_lib() && init_nvml()) {
    active_method = MonitorMethod::NVML;
    utilization.resize(max_samples);
    memory_info.resize(max_samples);
    debug("Successfully initialized NVIDIA monitoring via NVML");
    return;
  }

  if (test_nvidia_smi()) {
    active_method = MonitorMethod::SMI;
    debug("NVML initialization failed, falling back to nvidia-smi");
    return;
  }

  valid = false;
  active_method = MonitorMethod::NONE;
  warning("Both NVML and nvidia-smi failed. NVIDIA monitoring disabled.");
}

nvidiamon::~nvidiamon() {
  // nvmlShutdown only if fully initialised
  if (valid && active_method == MonitorMethod::NVML) {
    nvmlReturn_t result = nvmlShutdown();
    if (result != NVML_SUCCESS) {
      warning("nvmlShutdown was not successfully finished");
    }
  }

  if (nvml_handle) {
    dlclose(nvml_handle);
    nvml_handle = nullptr;
  }
}

std::pair<int, std::vector<std::string>> nvidiamon::read_gpu_stats_test(
    const std::string read_path) {
  std::vector<std::string> split_output{};
  std::string output;
  std::pair<int, std::vector<std::string>> ret{0, split_output};
  std::ifstream inp{read_path};
  while (std::getline(inp, output)) {
    split_output.push_back(output);
  }
  inp.close();
  ret.second = split_output;
  return ret;
}

// Query utilization and memory samples for a single GPU device.
// Returns false if the device should be skipped (fatal NVML error).
bool nvidiamon::query_device_samples(
    nvmlDevice_t device, unsigned int gpu_idx, unsigned int& util_count,
    unsigned int& mem_count, unsigned long long& current_max_timestamp) {
  util_count = max_samples;
  nvmlReturn_t result = nvmlDeviceGetProcessUtilization(
      device, utilization.data(), &util_count, last_seen_timestamp);

  if (result == NVML_ERROR_INSUFFICIENT_SIZE) {
    warning("Utilization sample buffer size (" + std::to_string(max_samples) +
            ") exceeded. Consider increasing max_samples.");
    return false;
  }

  if (result == NVML_ERROR_NOT_FOUND) {
    util_count = 0;
  } else if (result != NVML_SUCCESS) {
    warning("Failed to get process utilization for GPU index " +
            std::to_string(gpu_idx) + ": " +
            std::string(nvmlErrorString(result)));
    return false;
  }

  for (unsigned int i{0}; i < util_count; ++i) {
    if (utilization[i].timeStamp > current_max_timestamp) {
      current_max_timestamp = utilization[i].timeStamp;
    }
  }

  mem_count = max_samples;
  result = nvmlDeviceGetComputeRunningProcesses(device, &mem_count,
                                                memory_info.data());

  if (result == NVML_ERROR_INSUFFICIENT_SIZE) {
    warning("Memory sample buffer size (" + std::to_string(max_samples) +
            ") exceeded. Consider increasing max_samples.");
    return false;
  }

  if (result == NVML_ERROR_NOT_FOUND) {
    mem_count = 0;
  } else if (result != NVML_SUCCESS) {
    warning("Failed to get process memory utilization for GPU index " +
            std::to_string(gpu_idx) + ": " +
            std::string(nvmlErrorString(result)));
    return false;
  }

  return true;
}

// Match monitored PIDs against the sampled utilization and memory data,
// accumulating into the stats map.  Returns true if any PID matched.
bool nvidiamon::accumulate_process_stats(const std::vector<pid_t>& pids,
                                         unsigned int util_count,
                                         unsigned int mem_count,
                                         prmon::monitored_value_map& stats) {
  bool gpu_is_active{false};
  for (unsigned int target_pid : pids) {
    for (unsigned int i{0}; i < util_count; ++i) {
      if (utilization[i].pid == target_pid) {
        stats["gpusmpct"] += utilization[i].smUtil;
        stats["gpumempct"] += utilization[i].memUtil;
        gpu_is_active = true;
        break;
      }
    }
    for (unsigned int i{0}; i < mem_count; ++i) {
      if (memory_info[i].pid == target_pid) {
        stats["gpufbmem"] += (memory_info[i].usedGpuMemory / BYTES_PER_KB);
        gpu_is_active = true;
        break;
      }
    }
  }
  return gpu_is_active;
}

void nvidiamon::update_stats_nvml(const std::vector<pid_t>& pids,
                                  const std::string read_path) {
  prmon::monitored_value_map nvidia_stats_update{};
  for (const auto& value : nvidia_stats) {
    nvidia_stats_update[value.first] = 0L;
  }
  if (!valid) {
    return;
  }

  unsigned long long current_max_timestamp = last_seen_timestamp;
  unsigned int active_gpus{0};

  for (unsigned int gpu_idx{0}; gpu_idx < ngpus; ++gpu_idx) {
    nvmlDevice_t device;
    nvmlReturn_t result = nvmlDeviceGetHandleByIndex(gpu_idx, &device);
    if (result != NVML_SUCCESS) {
      warning("Failed to get handle for GPU index " + std::to_string(gpu_idx));
      continue;
    }

    unsigned int util_count{0};
    unsigned int mem_count{0};
    if (!query_device_samples(device, gpu_idx, util_count, mem_count,
                              current_max_timestamp)) {
      continue;
    }

    if (accumulate_process_stats(pids, util_count, mem_count,
                                 nvidia_stats_update)) {
      ++active_gpus;
    }
  }

  last_seen_timestamp = current_max_timestamp;
  nvidia_stats_update["ngpus"] = active_gpus;

  for (auto& value : nvidia_stats) {
    if (nvidia_stats_update.count(value.first)) {
      value.second.set_value(nvidia_stats_update[value.first]);
    }
  }
}

void nvidiamon::update_stats_smi(const std::vector<pid_t>& pids,
                                 const std::string read_path) {
  const std::vector<std::string> cmd = {"nvidia-smi", "pmon", "-s",
                                        "um",         "-c",   "1"};
  prmon::monitored_value_map nvidia_stats_update{};
  for (const auto& value : nvidia_stats) nvidia_stats_update[value.first] = 0L;

  std::pair<int, std::vector<std::string>> cmd_result;
  if (read_path.size()) {
    cmd_result = read_gpu_stats_test(read_path);
  } else {
    cmd_result = prmon::cmd_pipe_output(cmd);
    if (cmd_result.first) {
      // Failed
      error("Failed to execute 'nvidia-smi' to get GPU status (code " +
            std::to_string(cmd_result.first) + ")");
      return;
    }
  }
  if (log_level <= spdlog::level::debug) {
    std::stringstream strm;
    strm << "nvidiamon::update_stats got the following output ("
         << cmd_result.second.size() << "): " << std::endl;
    int i = 0;
    for (const auto& s : cmd_result.second) {
      strm << i << " -> " << s << std::endl;
      ++i;
    }
    debug(strm.str());
  }

  // Loop over output
  unsigned int gpu_idx{}, sm{}, mem{}, fb_mem{};
  std::string sm_s{}, mem_s{}, fb_mem_s{};
  pid_t pid{};
  std::string enc{}, dec{}, jpg{}, ofa{}, cg_type{}, ccpm{}, cmd_name{};
  std::unordered_map<unsigned int, bool>
      activegpus{};  // Avoid double counting active GPUs
  for (const auto& s : cmd_result.second) {
    if (s[0] == '#') continue;
    std::istringstream instr(s);
    instr >> gpu_idx >> pid >> cg_type >> sm_s >> mem_s >> enc >> dec >> jpg >>
        ofa >> fb_mem_s >> ccpm >> cmd_name;
    auto read_ok = !(instr.fail() || instr.bad());  // eof() is ok
    if (read_ok) {
      sm = prmon::parse_uint_field(sm_s);
      mem = prmon::parse_uint_field(mem_s);
      fb_mem = prmon::parse_uint_field(fb_mem_s);
      if (log_level <= spdlog::level::debug) {
        std::stringstream strm;
        strm << "Good read: " << gpu_idx << " " << pid << " " << cg_type << " "
             << sm << " " << mem << " " << enc << " " << dec << " " << jpg
             << " " << ofa << " " << fb_mem << " " << ccpm << " " << cmd_name
             << std::endl;
        debug(strm.str());
      }

      // Filter on PID value, so we only add stats for our processes
      for (auto const p : pids) {
        if (p == pid) {
          nvidia_stats_update["gpusmpct"] += sm;
          nvidia_stats_update["gpumempct"] += mem;
          nvidia_stats_update["gpufbmem"] += fb_mem * MB_to_KB;
          if (!activegpus.count(gpu_idx)) {
            ++nvidia_stats_update["ngpus"];
            activegpus[gpu_idx] = true;
          }
        }
      }

      // Now move summed stats to the persistent counters
      for (auto& value : nvidia_stats) {
        value.second.set_value(nvidia_stats_update[value.first]);
      }
    } else if (log_level <= spdlog::level::debug) {
      std::stringstream strm;
      strm << "Bad read of line: " << s << std::endl;
      strm << "Parsed to: " << gpu_idx << " " << pid << " " << cg_type << " "
           << sm << " " << mem << " " << enc << " " << dec << " " << jpg << " "
           << ofa << " " << fb_mem << " " << ccpm << " " << cmd_name
           << std::endl;

      strm << "StringStream status: good()=" << instr.good();
      strm << " eof()=" << instr.eof();
      strm << " fail()=" << instr.fail();
      strm << " bad()=" << instr.bad() << std::endl;
      debug(strm.str());
    }
  }

  if (log_level <= spdlog::level::debug) {
    std::stringstream strm;
    strm << "Parsed: ";
    for (const auto& value : nvidia_stats) {
      strm << value.first << ": " << value.second.get_value() << ";";
    }
    debug(strm.str());
  }
}

void nvidiamon::update_stats(const std::vector<pid_t>& pids,
                             const std::string read_path) {
  if (!valid && read_path.empty()) return;

  // Test mode: precooked data is always in SMI format
  if (read_path.size()) {
    update_stats_smi(pids, read_path);
    return;
  }

  switch (active_method) {
    case MonitorMethod::NVML:
      update_stats_nvml(pids, read_path);
      break;
    case MonitorMethod::SMI:
      update_stats_smi(pids, read_path);
      break;
    default:
      break;
  }
}

// Return NVIDIA stats
prmon::monitored_value_map const nvidiamon::get_text_stats() {
  prmon::monitored_value_map nvidia_stat_map{};
  for (const auto& value : nvidia_stats) {
    nvidia_stat_map[value.first] = value.second.get_value();
  }
  return nvidia_stat_map;
}

// For JSON return the peaks
prmon::monitored_value_map const nvidiamon::get_json_total_stats() {
  prmon::monitored_value_map nvidia_stat_map{};
  for (const auto& value : nvidia_stats) {
    nvidia_stat_map[value.first] = value.second.get_max_value();
  }
  return nvidia_stat_map;
}

// And the averages
prmon::monitored_average_map const nvidiamon::get_json_average_stats(
    unsigned long long elapsed_clock_ticks) {
  prmon::monitored_average_map nvidia_stat_map{};
  for (const auto& value : nvidia_stats) {
    nvidia_stat_map[value.first] = value.second.get_average_value();
  }
  return nvidia_stat_map;
}

bool nvidiamon::test_nvidia_smi() {
  const std::vector<std::string> cmd = {"nvidia-smi", "-L"};

  // The use of execvp means searching along PATH, which is fine
  // but does imply some extra stat() calls; nvidia-smi isn't going
  // to change location, so there is an argument for finding and
  // caching the directory path
  auto cmd_result = prmon::cmd_pipe_output(cmd);
  if (cmd_result.first != 0) return false;
  unsigned int gpus = 0;
  for (auto const& s : cmd_result.second) {
    // From C++20 can use 'starts_with'
    if (s.substr(0, 3).compare("GPU") == 0) {
      ++gpus;
    }
  }
  nvidiamon::ngpus = gpus;
  if (gpus == 0) {
    warning("Executed 'nvidia-smi -L', but no GPUs found");
    return false;
  } else if (gpus > 4) {
    warning(
        "More than 4 GPUs found, so GPU process monitoring will be unreliable");
  }
  return true;
}

// Open libnvidia-ml.so and resolve all function pointers
bool nvidiamon::load_nvml_lib() {
  nvml_handle = dlopen("libnvidia-ml.so", RTLD_NOW);
  if (!nvml_handle) {
    nvml_handle = dlopen("libnvidia-ml.so.1", RTLD_NOW);
  }
  if (!nvml_handle) {
    warning("Failed to load libnvidia-ml.so: " + std::string(dlerror()));
    return false;
  }

#define LOAD_SYM(var, sym, cast)       \
  var = (cast)dlsym(nvml_handle, sym); \
  if (!(var)) {                        \
    goto load_error;                   \
  }

  LOAD_SYM(nvmlInit, "nvmlInit", nvmlReturn_t (*)())
  LOAD_SYM(nvmlShutdown, "nvmlShutdown", nvmlReturn_t (*)())
  LOAD_SYM(nvmlErrorString, "nvmlErrorString", const char* (*)(nvmlReturn_t))
  LOAD_SYM(nvmlDeviceGetCount, "nvmlDeviceGetCount",
           nvmlReturn_t (*)(unsigned int*))
  LOAD_SYM(nvmlDeviceGetHandleByIndex, "nvmlDeviceGetHandleByIndex",
           nvmlReturn_t (*)(unsigned int, nvmlDevice_t*))
  LOAD_SYM(nvmlDeviceGetProcessUtilization, "nvmlDeviceGetProcessUtilization",
           nvmlReturn_t (*)(nvmlDevice_t, nvmlProcessUtilizationSample_t*,
                            unsigned int*, unsigned long long))
  LOAD_SYM(nvmlDeviceGetName, "nvmlDeviceGetName",
           nvmlReturn_t (*)(nvmlDevice_t, char*, unsigned int))
  LOAD_SYM(nvmlDeviceGetMaxClockInfo, "nvmlDeviceGetMaxClockInfo",
           nvmlReturn_t (*)(nvmlDevice_t, nvmlClockType_t, unsigned int*))
  LOAD_SYM(nvmlDeviceGetMemoryInfo, "nvmlDeviceGetMemoryInfo",
           nvmlReturn_t (*)(nvmlDevice_t, nvmlMemory_t*))

#undef LOAD_SYM

  // Try v3 first, then v2, then v1
  nvmlDeviceGetComputeRunningProcesses =
      (nvmlReturn_t (*)(nvmlDevice_t, unsigned int*, nvmlProcessInfo_v3_t*))
          dlsym(nvml_handle, "nvmlDeviceGetComputeRunningProcesses_v3");
  if (!nvmlDeviceGetComputeRunningProcesses) {
    nvmlDeviceGetComputeRunningProcesses =
        (nvmlReturn_t (*)(nvmlDevice_t, unsigned int*, nvmlProcessInfo_v3_t*))
            dlsym(nvml_handle, "nvmlDeviceGetComputeRunningProcesses_v2");
  }
  if (!nvmlDeviceGetComputeRunningProcesses) {
    nvmlDeviceGetComputeRunningProcesses =
        (nvmlReturn_t (*)(nvmlDevice_t, unsigned int*, nvmlProcessInfo_v3_t*))
            dlsym(nvml_handle, "nvmlDeviceGetComputeRunningProcesses");
  }
  if (!nvmlDeviceGetComputeRunningProcesses) {
    goto load_error;
  }
  return true;

load_error:
  warning("Failed to resolve NVML function symbols: " + std::string(dlerror()));
  dlclose(nvml_handle);
  nvml_handle = nullptr;
  return false;
}

bool nvidiamon::init_nvml() {
  nvmlReturn_t result = nvmlInit();
  if (result != NVML_SUCCESS) {
    warning("NVML Init failed: " + std::string(nvmlErrorString(result)));
    return false;
  }

  unsigned int gpus{};
  result = nvmlDeviceGetCount(&gpus);
  if (result != NVML_SUCCESS) {
    warning("Failed to get GPU count: " + std::string(nvmlErrorString(result)));
    nvmlShutdown();
    return false;
  }

  if (gpus == 0) {
    warning("nvmlInit() succeeded but no GPUs found");
    nvmlShutdown();
    return false;
  }
  ngpus = gpus;
  return true;
}

// Return the parameter list
prmon::parameter_list const nvidiamon::get_parameter_list() { return params; }

void const nvidiamon::get_hardware_info_nvml(nlohmann::json& hw_json) {
  hw_json["HW"]["gpu"]["nGPU"] = ngpus;
  for (unsigned int i{0}; i < ngpus; ++i) {
    nvmlDevice_t device;
    nvmlReturn_t result;
    nvmlMemory_t memInfo{};
    char name[NVML_DEVICE_NAME_BUFFER_SIZE] = {};
    unsigned int sm_freq{0};
    unsigned long long total_mem{0};

    result = nvmlDeviceGetHandleByIndex(i, &device);
    if (result != NVML_SUCCESS) {
      warning("Failed to get handle for GPU index " + std::to_string(i));
      continue;
    }

    result = nvmlDeviceGetName(device, name, NVML_DEVICE_NAME_BUFFER_SIZE);
    if (result != NVML_SUCCESS) {
      warning("Failed to get name for GPU index " + std::to_string(i));
      name[0] = '\0';
    }

    result = nvmlDeviceGetMaxClockInfo(device, NVML_CLOCK_SM, &sm_freq);
    if (result != NVML_SUCCESS) {
      warning("Failed to get SM frequency for GPU index " + std::to_string(i));
      sm_freq = 0;
    }

    result = nvmlDeviceGetMemoryInfo(device, &memInfo);
    if (result != NVML_SUCCESS) {
      warning("Failed to get memory info for GPU index " + std::to_string(i));
      memInfo.total = 0;
    }

    total_mem = memInfo.total / BYTES_PER_KB;

    std::string gpu_number = "gpu_" + std::to_string(i);
    hw_json["HW"]["gpu"][gpu_number]["name"] = name;
    hw_json["HW"]["gpu"][gpu_number]["sm_freq"] = sm_freq;
    hw_json["HW"]["gpu"][gpu_number]["total_mem"] = total_mem;
  }
}

// Collect related hardware information
void const nvidiamon::get_hardware_info_smi(nlohmann::json& hw_json) {
  // Record the number of GPUs
  hw_json["HW"]["gpu"]["nGPU"] = nvidiamon::ngpus;

  // For the GPUs present we get details using "nvidia-smi --query-gpu="
  // Note that the name is put at the end of the output as it makes
  // parsing easier
  std::vector<std::string> cmd = {
      "nvidia-smi", "--query-gpu=clocks.max.sm,memory.total,gpu_name",
      "--format=csv,noheader,nounits"};
  auto cmd_result = prmon::cmd_pipe_output(cmd);
  if (cmd_result.first) {
    error("Failed to get hardware details for GPUs");
    return;
  }
  unsigned int sm_freq, total_mem;
  unsigned int count{0};
  for (auto const& s : cmd_result.second) {
    std::istringstream instr(s);
    instr >> sm_freq;
    instr.get();  // Swallow the comma
    instr >> total_mem;
    auto pos = std::string::size_type(instr.tellg()) + 2;  // Skip ", "
    std::string name{"unknown"};
    if (pos <= s.size()) {  // This should never fail, but...
      name = s.substr(pos);
    }
    if (!(instr.fail() || instr.bad())) {
      std::string gpu_number = "gpu_" + std::to_string(count);
      hw_json["HW"]["gpu"][gpu_number]["name"] = name;
      hw_json["HW"]["gpu"][gpu_number]["sm_freq"] = sm_freq;
      hw_json["HW"]["gpu"][gpu_number]["total_mem"] = total_mem * MB_to_KB;
    } else {
      warning("Unexpected line from GPU hardware query: " + s);
    }
    ++count;
  }
  return;
}

void const nvidiamon::get_hardware_info(nlohmann::json& hw_json) {
  if (!valid) return;

  switch (active_method) {
    case MonitorMethod::NVML:
      get_hardware_info_nvml(hw_json);
      break;
    case MonitorMethod::SMI:
      get_hardware_info_smi(hw_json);
      break;
    default:
      break;
  }
}

void const nvidiamon::get_unit_info(nlohmann::json& unit_json) {
  prmon::fill_units(unit_json, params);
  return;
}
