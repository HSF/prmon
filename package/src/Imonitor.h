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

#include "parameter.h"

class Imonitor {
 public:
  virtual ~Imonitor(){};

  virtual void update_stats(const std::vector<pid_t>& pids,
                            const std::string read_path = "") = 0;

  virtual prmon::monitored_value_map const get_text_stats() = 0;
  virtual prmon::monitored_value_map const get_json_total_stats() = 0;
  virtual prmon::monitored_average_map const get_json_average_stats(
      prmon::mon_value elapsed_clock_ticks) = 0;
  virtual prmon::parameter_list const get_parameter_list() = 0;

  virtual void const get_hardware_info(nlohmann::json& hw_json) = 0;
  virtual void const get_unit_info(nlohmann::json& unit_json) = 0;
  virtual bool const is_valid() = 0;
};

#endif  // PRMON_IMONITOR_H
