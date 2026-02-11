// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

// Monitored quantity class

#include <map>
#include <string>
#include <vector>

#include "spdlog/spdlog.h"

#ifndef PRMON_PARAMETER_H
#define PRMON_PARAMETER_H 1

namespace prmon {
// Define here the types we use for each monitoring primitive
using mon_value = unsigned long long;
using avg_value = double;

using monitored_value_map = std::map<std::string, mon_value>;
using monitored_average_map = std::map<std::string, avg_value>;

// parameter class holds three strings for each monitored value:
// - the name of the value
// - the units of the value for maximums and averages
// as quite a few units don't have meaningful average values then
// these should be set to an empty string, which will suppress
// adding that information to the JSON output file
class parameter {
 private:
  std::string m_name;
  std::string m_max_unit, m_avg_unit;

 public:
  inline const std::string get_name() const { return m_name; }
  inline const std::string get_max_unit() const { return m_max_unit; }
  inline const std::string get_avg_unit() const { return m_avg_unit; }

  parameter(std::string n, std::string m, std::string a)
      : m_name{n}, m_max_unit{m}, m_avg_unit{a} {}
};

using parameter_list = std::vector<parameter>;

class monitored_value {
  // monitored_value class gives an interface to a value that we monitor
  // in prmon
  // It holds metadata about the value (name, units - using the parameter class)
  // as well as internal counters and accessors. In particular the monotonic
  // flag will prevent the measured value from being lowered.
 private:
  const parameter m_param;

  bool m_monotonic;
  unsigned long m_iterations;

  // All internal values will be stored with any offset applied
  mon_value m_value;
  mon_value m_max_value;
  mon_value m_summed_value;
  mon_value m_offset;

 public:
  inline const std::string get_name() const { return m_param.get_name(); }
  inline const std::string get_max_unit() const {
    return m_param.get_max_unit();
  }
  inline const std::string get_avg_unit() const {
    return m_param.get_avg_unit();
  }

  inline const mon_value get_value() const { return m_value; }
  inline const mon_value get_max_value() const { return m_max_value; }
  const mon_value get_summed_value() const;
  const avg_value get_average_value() const;
  inline const mon_value get_offset() const { return m_offset; }

  // set_value() should be called with the "raw" value and the monitor
  // will itself apply any offset needed
  int set_value(mon_value new_value);
  int set_offset(mon_value new_offset);

  monitored_value(parameter p, bool mono = false, mon_value offset = 0L)
      : m_param{p},
        m_monotonic{mono},
        m_iterations{0},
        m_value{0L},
        m_max_value{0L},
        m_summed_value{0L},
        m_offset{offset} {}
};

using monitored_list = std::map<std::string, prmon::monitored_value>;

}  // namespace prmon

#endif  // PRMON_PARAMETER_H
