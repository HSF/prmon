// Copyright (C) 2021 CERN
// License Apache2 - see LICENCE file

#include "parameter.h"

#include <iostream>

namespace prmon {
int monitored_value::set_value(mon_value new_value) {
  if (monotonic && (new_value < value)) {
    std::cerr << "Error: attempt to reduce the monitored value of " << param.get_name()
              << " (which is monotonic)" << std::endl;
    return 1;
  }
  value = new_value;
  if (value > max_value) max_value = value;
  return 0;
}
}  // namespace prmon
