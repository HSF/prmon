// Copyright (C) 2018-2021 CERN
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
// - the units of the value for maxiumums and averages
// as quite a few units don't have meaningful average values then
// these should be set to and empty string, which will suppress
// adding that informtion to the JSON output file
class parameter {
 private:
  std::string name;
  std::string max_unit, avg_unit;

 public:
  inline const std::string get_name() const { return name; }
  inline const std::string get_max_unit() const { return max_unit; }
  inline const std::string get_avg_unit() const { return avg_unit; }

  parameter(std::string n, std::string m, std::string a)
      : name{n}, max_unit{m}, avg_unit{a} {}
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
  mon_value value;
  mon_value max_value;
  mon_value summed_value;
  mon_value offset;

 public:
  inline const std::string get_name() const { return m_param.get_name(); }
  inline const std::string get_max_unit() const {
    return m_param.get_max_unit();
  }
  inline const std::string get_avg_unit() const {
    return m_param.get_avg_unit();
  }

  inline const mon_value get_value() const { return value; }
  inline const mon_value get_max_value() const { return max_value; }
  const mon_value get_summed_value() const;
  const avg_value get_average_value() const;
  inline const mon_value get_offset() const { return offset; }

  // set_value() should be called with the "raw" value and the monitor
  // will itself apply any offset needed
  int set_value(mon_value new_value);
  int set_offset(mon_value new_offset);

  monitored_value(std::string n, std::string m, std::string a,
                  bool mono = false, mon_value offset = 0L)
      : m_param{n, m, a},
        m_monotonic{mono},
        m_iterations{0},
        value{0L},
        max_value{0L},
        summed_value{0L},
        offset{offset} {}

  monitored_value(parameter p, bool mono = false, mon_value offset = 0L)
      : m_param{p},
        m_monotonic{mono},
        m_iterations{0},
        value{0L},
        max_value{0L},
        summed_value{0L},
        offset{offset} {}
};

using monitored_list = std::map<std::string, prmon::monitored_value>;

}  // namespace prmon

#endif  // PRMON_PARAMETER_H
