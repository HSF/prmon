#include <iostream>
#include <sstream>
#include <string>

#include "../../prmonVersion.h"
#include "../src/Imonitor.h"
#include "../src/cgroupmon.h"
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
      ASSERT_EQ(store_stats["gpusmpct"], 0);
      ASSERT_EQ(store_stats["gpumempct"], 0);
    }
  }
}

// Precooked cgroup v2 data for a process in the "/job" cgroup:
// iteration 1 has memory.max "max" and only kernel_stack (pre-5.18 kernel);
// iteration 2 has increased counters, a numeric memory.max, both kernel and
// kernel_stack, and two block devices in io.stat
TEST(CgroupmonTest, CgroupmonValueTestFixed) {
  std::string cur_path = base_path + "cgroup_v2";
  std::vector<pid_t> fake_pids = mother_pid;

  std::unique_ptr<Imonitor> monitor(
      registry::Registry<Imonitor>::create("cgroupmon"));

  // Cumulative counters are relative to the first reading
  const std::map<std::string, unsigned long long> expected[2] = {
      {{"cgroup_cpu_user", 0},
       {"cgroup_cpu_system", 0},
       {"cgroup_cpu_total", 0},
       {"cgroup_cpu_throttled", 0},
       {"cgroup_cpu_periods", 0},
       {"cgroup_io_read", 0},
       {"cgroup_io_write", 0},
       {"cgroup_mem_current", 150000},
       {"cgroup_mem_max", 0},
       {"cgroup_mem_anon", 100000},
       {"cgroup_mem_file", 50000},
       {"cgroup_mem_kernel", 1000},
       {"cgroup_nprocs", 1},
       {"cgroup_nthreads", 3}},
      {{"cgroup_cpu_user", 7000000},
       {"cgroup_cpu_system", 3000000},
       {"cgroup_cpu_total", 10000000},
       {"cgroup_cpu_throttled", 30000},
       {"cgroup_cpu_periods", 50},
       {"cgroup_io_read", 11534336},
       {"cgroup_io_write", 5242880},
       {"cgroup_mem_current", 300000},
       {"cgroup_mem_max", 1048576},
       {"cgroup_mem_anon", 200000},
       {"cgroup_mem_file", 100000},
       {"cgroup_mem_kernel", 20000},  // "kernel" only, not plus kernel_stack
       {"cgroup_nprocs", 2},
       {"cgroup_nthreads", 4}}};

  for (int iteration = 1; iteration <= 2; ++iteration) {
    std::stringstream iteration_path{};
    iteration_path << cur_path << "/" << iteration;
    monitor->update_stats(fake_pids, iteration_path.str());
    ASSERT_EQ(monitor->get_text_stats(), expected[iteration - 1])
        << "iteration " << iteration;
  }
  ASSERT_EQ(monitor->get_json_total_stats(), expected[1]);

  // Averages: only parameters with an average unit; B/s values are rates
  // over the elapsed time (10s here)
  const auto averages =
      monitor->get_json_average_stats(10 * sysconf(_SC_CLK_TCK));
  const std::map<std::string, double> expected_avg = {
      {"cgroup_io_read", 1153433.6},  {"cgroup_io_write", 524288.0},
      {"cgroup_mem_current", 225000}, {"cgroup_mem_anon", 150000},
      {"cgroup_mem_file", 75000},     {"cgroup_mem_kernel", 10500},
      {"cgroup_nprocs", 1.5},         {"cgroup_nthreads", 3.5}};
  ASSERT_EQ(averages.size(), expected_avg.size());
  for (const auto& avg : expected_avg) {
    ASSERT_EQ(averages.count(avg.first), 1u) << avg.first;
    EXPECT_DOUBLE_EQ(averages.at(avg.first), avg.second) << avg.first;
  }
}

// A pid without a readable /proc/<pid>/cgroup must not fall back to reading
// the root cgroup
TEST(CgroupmonTest, CgroupmonMissingPid) {
  std::unique_ptr<Imonitor> monitor(
      registry::Registry<Imonitor>::create("cgroupmon"));
  monitor->update_stats(std::vector<pid_t>{4242}, base_path + "cgroup_v2/1");
  for (const auto& stat : monitor->get_text_stats()) {
    ASSERT_EQ(stat.second, 0u) << stat.first;
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
