#include <iostream>
#include <sstream>
#include <string>

#include "../../prmonVersion.h"
#include "../src/Imonitor.h"
#include "../src/countmon.h"
#include "../src/cpumon.h"
#include "../src/iomon.h"
#include "../src/memmon.h"
#include "../src/netmon.h"
#include "../src/nvidiamon.h"
#include "../src/prmonutils.h"
#include "../src/registry.h"
#include "gtest/gtest.h"

const std::vector<pid_t> mother_pid{1729};

#define TO_STRING2(X) #X
#define TO_STRING(X) TO_STRING2(X)

const std::string base_path = TO_STRING(TESTS_SOURCE_DIR);

std::shared_ptr<spdlog::sinks::stdout_color_sink_st> c_sink{
    std::make_shared<spdlog::sinks::stdout_color_sink_st>()};
std::shared_ptr<spdlog::sinks::basic_file_sink_st> f_sink{
    std::make_shared<spdlog::sinks::basic_file_sink_st>("prmon.log", true)};

bool prmon::sigusr1 = false;

TEST(IomonTest, IomonMonitonicityTestFixed) {
  std::string cur_path = base_path + "drop";
  std::vector<pid_t> fake_pids = mother_pid;

  std::unique_ptr<Imonitor> monitor(
      registry::Registry<Imonitor>::create("iomon"));

  const int iterationCount = 3;

  std::map<std::string, unsigned long long> store_stats;
  for (int iteration = 1; iteration <= iterationCount; ++iteration) {
    std::stringstream iteration_path{};
    iteration_path << cur_path << "/" << iteration;
    monitor->update_stats(fake_pids, iteration_path.str());
    store_stats = monitor->get_text_stats();
    for (auto stat : store_stats) {
      if (iteration == 1) {
        ASSERT_EQ(stat.second, 500);
      } else if (iteration == 2) {
        ASSERT_EQ(stat.second, 1000);
      } else {
        ASSERT_EQ(stat.second, 1000);
      }
    }
  }
}

TEST(CpumonTest, CpumonMonitonicityTestFixed) {
  std::string cur_path = base_path + "drop";
  std::vector<pid_t> fake_pids = mother_pid;

  std::unique_ptr<Imonitor> monitor(
      registry::Registry<Imonitor>::create("cpumon"));

  const int iterationCount = 3;

  std::map<std::string, unsigned long long> store_stats;
  for (int iteration = 1; iteration <= iterationCount; ++iteration) {
    std::stringstream iteration_path{};
    iteration_path << cur_path << "/" << iteration;
    monitor->update_stats(fake_pids, iteration_path.str());
    store_stats = monitor->get_text_stats();
    for (auto stat : store_stats) {
      if (iteration == 1) {
        ASSERT_EQ(stat.second, (5000000 * 2) / sysconf(_SC_CLK_TCK));
      } else if (iteration == 2) {
        ASSERT_EQ(stat.second, (10000000 * 2) / sysconf(_SC_CLK_TCK));
      } else {
        ASSERT_EQ(stat.second, (10000000 * 2) / sysconf(_SC_CLK_TCK));
      }
    }
  }
}

TEST(MemmonTest, MemmonValueTestFixed) {
  std::string cur_path = base_path + "drop";
  std::vector<pid_t> fake_pids = mother_pid;

  std::unique_ptr<Imonitor> monitor(
      registry::Registry<Imonitor>::create("memmon"));

  const int iterationCount = 3;

  std::map<std::string, unsigned long long> store_stats;
  for (int iteration = 1; iteration <= iterationCount; ++iteration) {
    std::stringstream iteration_path{};
    iteration_path << cur_path << "/" << iteration;
    monitor->update_stats(fake_pids, iteration_path.str());
    store_stats = monitor->get_text_stats();
    for (auto stat : store_stats) {
      if (iteration == 1) {
        ASSERT_EQ(stat.second, 5000);
      } else if (iteration == 2) {
        ASSERT_EQ(stat.second, 10000);
      } else {
        ASSERT_EQ(stat.second, 2000);
      }
    }
  }
}

TEST(NetmonTest, NetmonMonitonicityTestFixed) {
  std::string cur_path = base_path + "drop";
  std::vector<pid_t> fake_pids = mother_pid;

  std::unique_ptr<Imonitor> monitor(
      registry::Registry<Imonitor, std::vector<std::string>>::create(
          "netmon", std::vector<std::string>()));

  const int iterationCount = 3;

  std::map<std::string, unsigned long long> store_stats;
  for (int iteration = 1; iteration <= iterationCount; ++iteration) {
    std::stringstream iteration_path{};
    iteration_path << cur_path << "/" << iteration << "/net/";
    monitor->update_stats(fake_pids, iteration_path.str());
    store_stats = monitor->get_text_stats();
    for (auto stat : store_stats) {
      if (iteration == 1) {
        ASSERT_EQ(stat.second, 500000);
      } else if (iteration == 2) {
        ASSERT_EQ(stat.second, 1000000);
      } else {
        ASSERT_EQ(stat.second, 1000000);
      }
    }
  }
}

TEST(NvidiamonTest, NvidiamonValueTestFixed) {
  std::string cur_path = base_path + "drop";
  std::vector<pid_t> fake_pids = mother_pid;

  std::unique_ptr<Imonitor> monitor(
      registry::Registry<Imonitor>::create("nvidiamon"));

  const int iterationCount = 3;

  std::map<std::string, unsigned long long> store_stats;
  const unsigned int MB_to_KB = 1024;
  for (int iteration = 1; iteration <= iterationCount; ++iteration) {
    std::stringstream iteration_path{};
    iteration_path << cur_path << "/" << iteration << "/nvidia/smi";
    monitor->update_stats(fake_pids, iteration_path.str());
    store_stats = monitor->get_text_stats();
    if (iteration == 1) {
      ASSERT_EQ(store_stats["gpufbmem"], 50 * MB_to_KB);
      ASSERT_EQ(store_stats["gpusmpct"], 50);
      ASSERT_EQ(store_stats["gpumempct"], 50);
    } else if (iteration == 2) {
      ASSERT_EQ(store_stats["gpufbmem"], 100 * MB_to_KB);
      ASSERT_EQ(store_stats["gpusmpct"], 100);
      ASSERT_EQ(store_stats["gpumempct"], 100);
    } else {
      ASSERT_EQ(store_stats["gpufbmem"], 20 * MB_to_KB);
      ASSERT_EQ(store_stats["gpusmpct"], 20);
      ASSERT_EQ(store_stats["gpumempct"], 20);
    }
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
