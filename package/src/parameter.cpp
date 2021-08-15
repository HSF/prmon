// Copyright (C) 2021 CERN
// License Apache2 - see LICENCE file

#include "parameter.h"

#include <iostream>

namespace prmon {
int monitored_value::set_value(mon_value new_value) {
  // If we have an offset for this parameter then passing in
  // a set value less than this is illegal
  if (new_value < offset) {
    spdlog::error("Error: attempt to set value of " + m_param.get_name() +
                  " to less than this parameter's offset");
    return 1;
  }
  new_value -= offset;

  if (m_monotonic) {
    // Monotonic values only increase and never have useful
    // sums or average values based on iterations
    if (new_value < value) {
      spdlog::error("Error: attempt to reduce the monitored value of " +
                    m_param.get_name() + " (which is monotonic)");
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
    ++m_iterations;
  }
  return 0;
}

int monitored_value::set_offset(mon_value const new_offset) {
  // It is only valid to apply this function when no iterations
  // have been made!
  offset = new_offset;
  if (m_iterations > 0) {
    spdlog::warn("Resetting the offset of measured values is dangerous (" +
                 m_param.get_name() + ")");
    return 1;
  }
  return 0;
}

prmon::mon_value const monitored_value::get_summed_value() const {
  if (!m_monotonic) {
    return summed_value;
  }
  return 0;
}

prmon::avg_value const monitored_value::get_average_value() const {
  if ((!m_monotonic) && (m_iterations > 0)) {
    return prmon::avg_value(summed_value) / m_iterations;
  }
  return 0;
}

}  // namespace prmon
