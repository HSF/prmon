// Unit tests for parameter classes in parameter.h

#include <string>

#include "../src/parameter.h"
#include "gtest/gtest.h"

const std::string param_name{"TestValue"};
const std::string param_unit{"MegaFloops"};
const std::string param_avg_unit{"MegaFloops/s"};

// Simple test of parameter class
TEST(ParameterClass, InitAndGetValues) {
  prmon::parameter test_param{param_name, param_unit, param_avg_unit};

  EXPECT_EQ(test_param.get_name(), param_name);
  EXPECT_EQ(test_param.get_max_unit(), param_unit);
  EXPECT_EQ(test_param.get_avg_unit(), param_avg_unit);
}

// Fixture class to define some different class instances for testing
class MoitoredValueClassTest : public ::testing::Test {
 protected:
  // void SetUp() override {}
  // void TearDown() override {}

  const prmon::mon_value value1{668};
  const prmon::mon_value value2{670};
  const prmon::mon_value value3{662};

  const prmon::mon_value offset{665};

  prmon::parameter test_param{param_name, param_unit, param_avg_unit};
  prmon::monitored_value test_value{test_param};
  prmon::monitored_value test_mono_value{test_param, true};
  prmon::monitored_value test_offset_value{test_param, false, offset};
  prmon::monitored_value test_mono_offset_value{test_param, true, offset};
};

// Test basic setup of monitored_value
TEST_F(MoitoredValueClassTest, InitAndGetValues) {
  EXPECT_EQ(test_value.get_name(), param_name);
  EXPECT_EQ(test_value.get_max_unit(), param_unit);
  EXPECT_EQ(test_value.get_avg_unit(), param_avg_unit);
}

// Normal parameter, which can go up and down
TEST_F(MoitoredValueClassTest, SetNormalValues) {
  EXPECT_EQ(test_value.set_value(value1), 0);
  EXPECT_EQ(test_value.get_value(), value1);

  EXPECT_EQ(test_value.set_value(value2), 0);
  EXPECT_EQ(test_value.get_value(), value2);

  EXPECT_EQ(test_value.set_value(value3), 0);
  EXPECT_EQ(test_value.get_value(), value3);

  EXPECT_EQ(test_value.get_max_value(), value2);
  EXPECT_EQ(test_value.get_summed_value(), value1 + value2 + value3);
  EXPECT_EQ(test_value.get_average_value(),
            prmon::avg_value(value1 + value2 + value3) / 3);
}

// Monotonic paramater, cannot go down
TEST_F(MoitoredValueClassTest, SetMonoValues) {
  EXPECT_EQ(test_mono_value.set_value(value1), 0);
  EXPECT_EQ(test_mono_value.get_value(), value1);

  EXPECT_EQ(test_mono_value.set_value(value2), 0);
  EXPECT_EQ(test_mono_value.get_value(), value2);

  EXPECT_EQ(test_mono_value.set_value(value3), 1);
  EXPECT_EQ(test_mono_value.get_value(), value2);

  EXPECT_EQ(test_mono_value.get_max_value(), value2);
  EXPECT_EQ(test_mono_value.get_summed_value(), 0);
  EXPECT_EQ(test_mono_value.get_average_value(), 0);
}

// Normal value, with an offset
TEST_F(MoitoredValueClassTest, SetOffsetValues) {
  EXPECT_EQ(test_offset_value.set_value(value1), 0);
  EXPECT_EQ(test_offset_value.get_value(), value1 - offset);

  EXPECT_EQ(test_offset_value.set_value(value2), 0);
  EXPECT_EQ(test_offset_value.get_value(), value2 - offset);

  EXPECT_EQ(test_offset_value.set_value(value3), 1);
  EXPECT_EQ(test_offset_value.get_value(), value2 - offset);

  EXPECT_EQ(test_offset_value.get_max_value(), value2 - offset);
  EXPECT_EQ(test_offset_value.get_summed_value(), value1 + value2 - 2 * offset);
  EXPECT_EQ(test_offset_value.get_average_value(),
            prmon::avg_value(value1 + value2 - 2 * offset) / 2);
}

// Monotonic, with an offset
TEST_F(MoitoredValueClassTest, SetMonoOffsetValues) {
  EXPECT_EQ(test_mono_offset_value.set_value(value1), 0);
  EXPECT_EQ(test_mono_offset_value.get_value(), value1 - offset);

  EXPECT_EQ(test_mono_offset_value.set_value(value2), 0);
  EXPECT_EQ(test_mono_offset_value.get_value(), value2 - offset);

  EXPECT_EQ(test_mono_offset_value.set_value(value3), 1);
  EXPECT_EQ(test_mono_offset_value.get_value(), value2 - offset);

  EXPECT_EQ(test_mono_offset_value.get_max_value(), value2 - offset);
  EXPECT_EQ(test_mono_offset_value.get_summed_value(), 0);
  EXPECT_EQ(test_mono_offset_value.get_average_value(), 0);
}
