#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
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

#define TO_STRING2(X) #X
#define TO_STRING(X) TO_STRING2(X)

const std::string base_path = TO_STRING(PRMON_SOURCE_DIR);
const std::string prmon_path = base_path + "/package/prmon";

bool prmon::sigusr1 = false;

int run_prmon(const std::vector<std::string>& disabled_monitors) {
  pid_t parent_pid = getpid();
  pid_t pid = fork();
  if (pid < 0) {
    return 1;
  }
  if (pid > 0) {
    sleep(3);
  } else {
    std::vector<std::string> arg_list = {prmon_path, "--pid",
                                         std::to_string(parent_pid)};
    for (const auto& monitor : disabled_monitors) {
      arg_list.push_back("--disable");
      arg_list.push_back(monitor);
    }
    int exec_argc = arg_list.size() + 1;
    char** exec_argv = (char**)malloc(exec_argc * sizeof(char*));

    for (int i = 0; i < exec_argc - 1; ++i) {
      exec_argv[i] = (char*)arg_list[i].c_str();
    }
    exec_argv[exec_argc - 1] = NULL;
    execvp(exec_argv[0], exec_argv);
  }
  kill(pid, SIGTERM);
  return 0;
}

std::vector<std::string> get_enabled_monitors(
    const std::vector<std::string>& disabled_monitors) {
  auto registered_monitors = prmon::get_all_registered();
  std::vector<std::string> enabled_monitors;
  for (const auto& monitor : registered_monitors) {
    bool enabled = true;
    for (const auto& disabled_monitor : disabled_monitors) {
      if (monitor == disabled_monitor) {
        enabled = false;
        break;
      }
    }
    if (enabled) enabled_monitors.push_back(monitor);
  }
  return enabled_monitors;
}

std::unordered_map<std::string, prmon::parameter_list> get_parameters(
    const std::vector<std::string>& disabled_monitors) {
  std::unordered_map<std::string, prmon::parameter_list> monitor_params{};
  std::vector<std::string> netdevs;
  auto registered_monitors = prmon::get_all_registered();
  for (const auto& class_name : registered_monitors) {
    // Check if the monitor should be enabled
    bool state = true;
    for (const auto& disabled : disabled_monitors) {
      if (class_name == disabled) state = false;
    }
    if (state) {
      std::unique_ptr<Imonitor> new_monitor_p;
      if (class_name == "netmon") {
        new_monitor_p = std::unique_ptr<Imonitor>(
            registry::Registry<Imonitor, std::vector<std::string>>::create(
                class_name, netdevs));
      } else {
        new_monitor_p = std::unique_ptr<Imonitor>(
            registry::Registry<Imonitor>::create(class_name));
      }
      if (new_monitor_p) {
        if (new_monitor_p->is_valid()) {
          monitor_params[class_name] = new_monitor_p->get_parameter_list();
        }
      } else {
        spdlog::error("Registration of monitor " + class_name + " FAILED");
      }
    }
  }
  return monitor_params;
}

std::unordered_map<std::string, std::vector<std::string>> get_json_params() {
  nlohmann::json json_file;
  std::ifstream f("prmon.json_snapshot");
  f >> json_file;
  std::unordered_map<std::string, std::vector<std::string>> params;
  for (const auto& elem : json_file.items()) {
    if (elem.key() == "Avg" || elem.key() == "Max") {
      for (const auto& field : elem.value().items()) {
        params[elem.key()].push_back(field.key());
      }
    }
  }
  return params;
}

std::vector<std::string> get_prmon_txt_params() {
  std::ifstream f("prmon.txt");
  std::string line, field;
  std::getline(f, line);
  std::stringstream field_stream(line);
  std::vector<std::string> field_list;
  while (field_stream >> field) field_list.emplace_back(field);
  return field_list;
}

std::unordered_map<std::string, prmon::parameter_list> monitor_params;

TEST(JsonTest, JsonFieldsTest) {
  const auto& json_params = get_json_params();
  for (const auto& monitor_param : monitor_params) {
    for (const auto& param : monitor_param.second) {
      bool avg_found = false, max_found = false;
      for (const auto& json_param : json_params.at("Avg")) {
        if (param.get_name() == json_param) {
          avg_found = true;
          break;
        }
      }
      for (const auto& json_param : json_params.at("Max")) {
        if (param.get_name() == json_param) {
          max_found = true;
          break;
        }
      }
      if (param.get_name().size() < 4 ||
          param.get_name().substr((int)param.get_name().size() - 4, 4) !=
              "time") {
        ASSERT_EQ(avg_found, true)
            << param.get_name() << " from " << monitor_param.first
            << " not found in json averages\n";
      }
      ASSERT_EQ(max_found, true)
          << param.get_name() << " from " << monitor_param.first
          << " not found in json maximums\n";
    }
  }
}

TEST(TxtTest, TxtFieldsTest) {
  const auto& txt_params = get_prmon_txt_params();
  for (const auto& monitor_param : monitor_params) {
    for (const auto& param : monitor_param.second) {
      bool found = false;
      for (const auto& txt_param : txt_params) {
        if (param.get_name() == txt_param) {
          found = true;
          break;
        }
      }
      ASSERT_EQ(found, true)
          << param.get_name() << " from " << monitor_param.first
          << " not found in txt fields\n";
    }
  }
}
int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  static struct option long_options[] = {
      {"disable", required_argument, NULL, 'd'}, {0, 0, 0, 0}};
  std::vector<std::string> disabled_monitors;
  int c;
  while ((c = getopt_long(argc, argv, "-d:", long_options, NULL)) != -1) {
    switch (c) {
      case 'd':
        if (prmon::valid_monitor_disable(optarg))
          disabled_monitors.push_back(optarg);
        break;
      default:
        std::cout << "Invalid option!" << std::endl;
        return 1;
    }
  }
  run_prmon(disabled_monitors);
  monitor_params = get_parameters(disabled_monitors);
  return RUN_ALL_TESTS();
}
