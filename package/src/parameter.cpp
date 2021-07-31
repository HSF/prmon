// Copyright (C) 2021 CERN
// License Apache2 - see LICENCE file

#include "parameter.h"

#include <iostream>

namespace prmon {
int monitored_value::set_value(mon_value new_value) {
  if (monotonic) {
    // Monotonic values only increase and never have useful
    // sums or average values based on iterations
    if (new_value < value) {
      std::cerr << "Error: attempt to reduce the monitored value of "
              << param.get_name() << " (which is monotonic)" << std::endl;
      return 1;
    }
    value = new_value;
    max_value = new_value;
  } else {
    // Non-monotonic values can go up and down and have
    // useful average values
    value = new_value;
    if (value > max_value) max_value = value;
    summed_value += value;
    ++iterations;
  }
  return 0;
}

prmon::mon_value const monitored_value::get_summed_value() const {
  if (!monotonic) {
    return summed_value;
  }
  return 0;
}

prmon::avg_value const monitored_value::get_average_value() const {
  if ((!monotonic) && (iterations > 0)) {
    return prmon::avg_value(summed_value) / iterations;
  }
  return 0;
}

}  // namespace prmon
