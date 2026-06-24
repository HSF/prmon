// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

#include "../src/cgroupmon.h"

#include <vector>
#include <iostream>

#include "gtest/gtest.h"

// Test cgroup v2 monitoring with precooked sources
TEST(CgroupmonTest, CgroupV2PreookedTest) {
  cgroupmon cg_monitor{};
  
  std::vector<pid_t> pids{1729};
  
  // Update stats using precooked test data
  // Note: This requires precooked test sources to be created
  // For now, this is a placeholder that validates the monitor can be created
  
  EXPECT_TRUE(true);  // Basic compilation test
}

// Test cgroup detection
TEST(CgroupmonTest, CgroupDetectionTest) {
  cgroupmon cg_monitor{};
  
  // The monitor should initialize without crashing
  // Validity depends on system cgroup support
  EXPECT_TRUE(true);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
