// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

// Monitored quantity class

#include <string>
#include <vector>

#ifndef PRMON_PARAMETER_H
#define PRMON_PARAMETER_H 1

namespace prmon {
  class parameter {
   private:
    std::string name;
    std::string max_unit, avg_unit;

   public:
    inline const std::string get_name() const {return name;}
    inline const std::string get_max_unit() const {return max_unit;}
    inline const std::string get_avg_unit() const {return avg_unit;}

    parameter(std::string n, std::string m, std::string a) :
    name{n}, max_unit{m}, avg_unit{a} {}
  };

  using parameter_list = std::vector<parameter>;
}

#endif  // PRMON_PARAMETER_H
