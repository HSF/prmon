// Copyright (C) 2020-2025 CERN

#include "nvidiamon.h"

#include <stdlib.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "utils.h"

#define MONITOR_NAME "nvidiamon"

// Constructor; uses RAII pattern to be valid after construction
nvidiamon::nvidiamon() {
  log_init(MONITOR_NAME);
#undef MONITOR_NAME
  for (const auto& param : params) {
    nvidia_stats.emplace(param.get_name(), prmon::monitored_value(param));
  }

  // Attempt to execute nvidia-smi
  // If this works we are valid, but if not then we can't get data
  valid = test_nvidia_smi();
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

void nvidiamon::update_stats(const std::vector<pid_t>& pids,
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
  pid_t pid{};
  std::string enc{}, dec{}, jpg{}, ofa{}, cg_type{}, ccpm{}, cmd_name{};
  std::unordered_map<unsigned int, bool>
      activegpus{};  // Avoid double counting active GPUs
  for (const auto& s : cmd_result.second) {
    if (s[0] == '#') continue;
    std::istringstream instr(s);
    instr >> gpu_idx >> pid >> cg_type >> sm >> mem >> enc >> dec >> jpg >> ofa >> fb_mem >>
          ccpm  >> cmd_name;
    auto read_ok = !(instr.fail() || instr.bad());  // eof() is ok
    if (read_ok) {
      if (log_level <= spdlog::level::debug) {
        std::stringstream strm;
        strm << "Good read: " << gpu_idx << " " << pid << " " << cg_type << " "
             << sm << " " << mem << " " << enc << " " << dec << " " << jpg << " " << ofa << " " << fb_mem << " " << ccpm
             << " " << cmd_name << std::endl;
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
           << sm << " " << mem << " " << enc << " " << dec << " " << jpg << " " << ofa << " " << fb_mem << " " << ccpm
           << " " << cmd_name << std::endl;

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

// Return the parameter list
prmon::parameter_list const nvidiamon::get_parameter_list() { return params; }

// Collect related hardware information
void const nvidiamon::get_hardware_info(nlohmann::json& hw_json) {
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

void const nvidiamon::get_unit_info(nlohmann::json& unit_json) {
  prmon::fill_units(unit_json, params);
  return;
}
