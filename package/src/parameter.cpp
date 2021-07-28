// Copyright (C) 2021 CERN
// License Apache2 - see LICENCE file

#include <iostream>

#include "parameter.h"

namespace prmon {
  int monitored_value::set_value(mon_value new_value) {
    if (monotonic && (new_value < value)) {
      std::cerr << "Error: attempt to reduce the monitored value of " << name << " (which is monotonic)" << std::endl;
      return 1;
    }
    value = new_value;
    return 0;
  }
}
