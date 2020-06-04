// Copyright (C) CERN, 2018
//
// Network monitoring interface class
//
// This is the interface class for all concrete monitoring
// classes

#ifndef PRMON_IMONITOR_H
#define PRMON_IMONITOR_H 1

#include <sys/types.h>
#include <time.h>

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

class Imonitor {
 public:
  virtual ~Imonitor() {};

  virtual void update_stats(const std::vector<pid_t>& pids) = 0;

  virtual std::map<std::string, unsigned long long> const get_text_stats() = 0;
  virtual std::map<std::string, unsigned long long> const get_json_total_stats() = 0;
  virtual std::map<std::string, double> const get_json_average_stats(unsigned long long elapsed_clock_ticks) = 0;

  virtual void const get_hardware_info(nlohmann::json& j) = 0;
};

#endif  // PRMON_IMONITOR_H
