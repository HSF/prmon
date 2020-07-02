// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

// PRocess MONitoring interface class
//
// This is the interface class for all concrete monitoring
// classes

#ifndef PRMON_IMONITOR_H
#define PRMON_IMONITOR_H 1

#include <sys/types.h>
#include <time.h>

#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

class Imonitor {
 public:
  virtual ~Imonitor(){};

  virtual void update_stats(const std::vector<pid_t>& pids) = 0;

  virtual std::map<std::string, unsigned long long> const get_text_stats() = 0;
  virtual std::map<std::string, unsigned long long> const
  get_json_total_stats() = 0;
  virtual std::map<std::string, double> const get_json_average_stats(
      unsigned long long elapsed_clock_ticks) = 0;

  virtual void const get_hardware_info(nlohmann::json& hw_json) = 0;
  virtual void const get_unit_info(nlohmann::json& unit_json) = 0;
  virtual bool const is_valid() = 0;
};

#endif  // PRMON_IMONITOR_H
