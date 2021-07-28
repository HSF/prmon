// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

// Monitored quantity class

#include <string>
#include <vector>

#ifndef PRMON_PARAMETER_H
#define PRMON_PARAMETER_H 1

// Define here the types we use for each monitoring primitive
using mon_value = unsigned long long;
using avg_value = double;

namespace prmon {
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
  // It holds metadata about the value (name, units) as well as internal
  // counters and accessors. In particular the monotonic flag will prevent
  // the measured value from being lowered.
 private:
  std::string name;
  std::string max_unit, avg_unit;

  bool monotonic;
  mon_value value;

 public:
  monitored_value(std::string n, std::string m, std::string a,
                  mon_value start = 0L)
      : name{n}, max_unit{m}, avg_unit{a}, value{start} {}

  inline const std::string get_name() const { return name; }
  inline const std::string get_max_unit() const { return max_unit; }
  inline const std::string get_avg_unit() const { return avg_unit; }

  inline const mon_value get_value() const { return value; }

  int set_value(mon_value new_value);
};

using monitored_list = std::map<std::string, monitored_value>;

}  // namespace prmon

#endif  // PRMON_PARAMETER_H
