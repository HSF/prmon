// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

// Monitored quantity class

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

#ifndef PRMON_PARAMETER_H
#define PRMON_PARAMETER_H 1

namespace prmon {
// parameter class holds three strings for each monitored value:
// - the name of the value
// - the units of the value for maxiumums and averages
// as quite a few units don't have meaningful average values then
// these should be set to and empty string, which will suppress
// adding that informtion to the JSON output file

using parameter_value = unsigned long long;

class parameter {
 private:
  // Parameters set at initialisation
  std::string name;
  std::string max_unit, avg_unit;
  bool monotonic;

  // Parameters used to count over time
  parameter_value current;
  parameter_value previous;
  parameter_value offset;

 public:
  inline const std::string get_name() const { return name; }
  inline const std::string get_max_unit() const { return max_unit; }
  inline const std::string get_avg_unit() const { return avg_unit; }
  inline const bool get_monotonic() const { return monotonic; }

  inline const parameter_value get_value() const {return current - offset; }
  inline const parameter_value get_current() const {return current; }
  inline const parameter_value get_offset() const {return offset; }

  int set_current(parameter_value new_value) {
    if (monotonic && new_value < current) {
      std::clog << "Warning: attempt to set monotonic metric '" << name <<  "'from " << current << " to " << new_value << std::endl;
      return 1;
    }
    current = new_value;
    return 0;
  }

  inline void set_offset(parameter_value new_offset) { offset = new_offset; }

  parameter(std::string n, std::string m, std::string a, bool mono)
      : name{n}, max_unit{m}, avg_unit{a}, monotonic{mono}, current{0}, previous{0}, offset{0} {}
};

using parameter_list = std::vector<parameter>;
using parameter_map = std::unordered_map<std::string, parameter>;
}  // namespace prmon

#endif  // PRMON_PARAMETER_H
