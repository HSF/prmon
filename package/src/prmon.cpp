// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

// PRocess MONitor
// See https://github.com/HSF/prmon

#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

#include "prmonVersion.h"
#include "prmonutils.h"
#include "registry.h"
#include "wallmon.h"

bool prmon::sigusr1 = false;

int ProcessMonitor(const pid_t mpid, const std::string filename,
                   const std::string json_summary_file, const time_t interval,
                   const bool store_hw_info, const bool store_unit_info,
                   const std::vector<std::string> netdevs,
                   const std::vector<std::string> disabled_monitors) {
  signal(SIGUSR1, prmon::SignalCallbackHandler);

  // This is the vector of all monitoring components
  std::unordered_map<std::string, std::unique_ptr<Imonitor>> monitors{};

  auto registered_monitors = registry::Registry<Imonitor>::list_registered();
  for (const auto& class_name : registered_monitors) {
    // Check if the monitor should be enabled
    bool state = true;
    for (const auto& disabled : disabled_monitors) {
      if (class_name == disabled) state = false;
    }
    if (state) {
      std::unique_ptr<Imonitor> new_monitor_p(
          registry::Registry<Imonitor>::create(class_name));
      if (new_monitor_p) {
        if (new_monitor_p->is_valid()) {
          monitors[class_name] = std::move(new_monitor_p);
        }
      } else {
        std::cerr << "Registration of monitor " << class_name << " FAILED"
                  << std::endl;
      }
    }
  }
  // The wallclock monitor is always needed as it is used for average stat
  // generation - wallmon cannot be disabled, but this is prechecked in
  // prmon::valid_monitor_disable before we get here
  if (monitors.count("wallmon") != 1) {
    std::cerr << "Failed to initialise mandatory wallclock monitoring class"
              << std::endl;
    exit(EXIT_FAILURE);
  }

  int iteration = 0;
  time_t lastIteration = time(0) - interval;
  time_t currentTime;

  // Open iteration output file
  std::ofstream file;
  file.open(filename);
  file << "Time";
  for (const auto& monitor : monitors) {
    for (const auto& stat : monitor.second->get_text_stats())
      file << "\t" << stat.first;
  }
  file << std::endl;

  // Create an empty JSON structure
  nlohmann::json json_summary;

  std::stringstream tmp_json_file;
  tmp_json_file << json_summary_file << "_tmp";
  std::stringstream json_snapshot_file;
  json_snapshot_file << json_summary_file << "_snapshot";

  // Add prmon version file to the JSON
  json_summary["prmon"]["Version"] = prmon_VERSION;

  // Collect some hardware information first (if requested)
  if (store_hw_info) {
    for (const auto& monitor : monitors)
      monitor.second->get_hardware_info(json_summary);
  }
  if (store_unit_info) {
    for (const auto& monitor : monitors)
      monitor.second->get_unit_info(json_summary);
  }

  // See if the kernel is new enough to have /proc/PID/task/PID/children
  bool modern_kernel = prmon::kernel_proc_pid_test(mpid);

  // Monitoring loop until process exits
  bool wroteFile = false;
  std::vector<pid_t> cpids{};
  int return_code = 0;
  // Scope of 'monitors' ensures safety of bare pointer here
  auto wallclock_monitor_p = static_cast<wallmon*>(monitors["wallmon"].get());
  while (kill(mpid, 0) == 0 && prmon::sigusr1 == false) {
    if (time(0) - lastIteration > interval) {
      iteration++;
      // Reset lastIteration
      lastIteration = time(0);

      if (modern_kernel)
        cpids = prmon::offspring_pids(mpid);
      else
        cpids = prmon::pstree_pids(mpid);

      try {
        for (const auto& monitor : monitors)
          monitor.second->update_stats(cpids);

        currentTime = time(0);
        file << currentTime;
        for (const auto& monitor : monitors) {
          for (const auto& stat : monitor.second->get_text_stats())
            file << "\t" << stat.second;
        }
        file << std::endl;

        // Create JSON realtime summary
        for (const auto& monitor : monitors) {
          for (const auto& stat : monitor.second->get_json_total_stats()) {
            json_summary["Max"][(stat.first).c_str()] = stat.second;
          }
        }
        for (const auto& monitor : monitors) {
          auto wallclock_time = wallclock_monitor_p->get_wallclock_clock_t();
          for (const auto& stat :
               monitor.second->get_json_average_stats(wallclock_time)) {
            // We will limit the decimal place accuracy here as it doesn't
            // make any sense to provide tens of decimal places of
            // precision (this is a limitation of the nlohmann::json
            // library, which is always striving for minimal precision
            // loss when converting float types to decimal strings)
            double integer_piece;
            // Volatile to prevent optimisation if performing rounding
            volatile double fractional_piece;
            fractional_piece = std::modf(stat.second, &integer_piece);
            // The rounding method of casting to int, then back to double is
            // expensive, so try to avoid it
            if (integer_piece > prmon::avg_precision ||
                fractional_piece == 0.0) {
              json_summary["Avg"][(stat.first).c_str()] = integer_piece;
            } else {
              fractional_piece =
                  static_cast<int>(fractional_piece * prmon::avg_precision) /
                  static_cast<double>(prmon::avg_precision);
              json_summary["Avg"][(stat.first).c_str()] =
                  integer_piece + fractional_piece;
            }
          }
        }

        // Write JSON realtime summary to a temporary file
        std::ofstream json_out(tmp_json_file.str());
        json_out << std::setw(2) << json_summary << std::endl;
        json_out.close();
        wroteFile = true;

        // Move temporary file to the snapshot file
        if (wroteFile) {
          if (rename(tmp_json_file.str().c_str(),
                     json_snapshot_file.str().c_str()) != 0) {
            perror("rename fails");
            std::cerr << tmp_json_file.str() << " " << json_snapshot_file.str()
                      << "\n";
          }
        }
      } catch (const std::ifstream::failure& e) {
        // Serious problem reading one of the status files, usually
        // caused by a child exiting during the poll - just try again
        // next time
        std::clog << "prmon ifstream exception: " << e.what() << " (ignored)"
                  << std::endl;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return_code = prmon::reap_children();
  }
  file.close();

  // Cleanup snapshot file
  if (remove(json_snapshot_file.str().c_str()) != 0 && iteration > 0)
    perror("remove fails");

  // Write final JSON summary file
  file.open(json_summary_file);
  file << std::setw(2) << json_summary << std::endl;
  file.close();

  // Check that we ran for a reasonable number of iterations
  if (wallclock_monitor_p->get_wallclock_clock_t() /
          (interval * sysconf(_SC_CLK_TCK)) <
      1) {
    std::clog << "Wallclock time of monitored process was less than the "
                 "monitoring interval, so average statistics will be unreliable"
              << std::endl;
  }

  return return_code;
}

int main(int argc, char* argv[]) {
  // Set defaults
  const char* default_filename = "prmon.txt";
  const char* default_json_summary = "prmon.json";
  const unsigned int default_interval = 30;
  const bool default_store_hw_info = true;
  const bool default_store_unit_info = false;

  pid_t pid = -1;
  bool got_pid = false;
  std::string filename{default_filename};
  std::string jsonSummary{default_json_summary};
  std::vector<std::string> netdevs{};
  std::vector<std::string> disabled_monitors{};
  unsigned int interval{default_interval};
  bool store_hw_info{default_store_hw_info};
  bool store_unit_info{default_store_unit_info};
  int do_help{0};

  static struct option long_options[] = {
      {"pid", required_argument, NULL, 'p'},
      {"filename", required_argument, NULL, 'f'},
      {"json-summary", required_argument, NULL, 'j'},
      {"interval", required_argument, NULL, 'i'},
      {"disable", required_argument, NULL, 'd'},
      {"suppress-hw-info", no_argument, NULL, 's'},
      {"units", no_argument, NULL, 'u'},
      {"netdev", required_argument, NULL, 'n'},
      {"help", no_argument, NULL, 'h'},
      {0, 0, 0, 0}};

  int c;
  while ((c = getopt_long(argc, argv, "p:f:j:i:d:sun:h", long_options, NULL)) !=
         -1) {
    switch (char(c)) {
      case 'p':
        pid = std::stoi(optarg);
        got_pid = true;
        break;
      case 'f':
        filename = optarg;
        break;
      case 'j':
        jsonSummary = optarg;
        break;
      case 'i':
        interval = std::stoi(optarg);
        break;
      case 's':
        store_hw_info = false;
        break;
      case 'u':
        store_unit_info = true;
        break;
      case 'n':
        netdevs.push_back(optarg);
        break;
      case 'd':
        // Check we got a valid monitor name
        if (prmon::valid_monitor_disable(optarg))
          disabled_monitors.push_back(optarg);
        break;
      case 'h':
        do_help = 1;
        break;
      default:
        std::cerr << "Use '--help' for usage " << std::endl;
        return 1;
    }
  }

  if (do_help) {
    std::cout
        << "prmon is a process monitor program that records runtime data\n"
        << "from a process and its children, writing time stamped values\n"
        << "for resource consumption into a logfile and a JSON summary\n"
        << "format when the process exits.\n"
        << std::endl;
    std::cout
        << "Options:\n"
        << "[--pid, -p PID]           Monitored process ID\n"
        << "[--filename, -f FILE]     Filename for detailed stats (default "
        << default_filename << ")\n"
        << "[--json-summary, -j FILE] Filename for JSON summary (default "
        << default_json_summary << ")\n"
        << "[--interval, -i TIME]     Seconds between samples (default "
        << default_interval << ")\n"
        << "[--suppress-hw-info, -s]  Disable hardware information (default "
        << (default_store_hw_info ? "false" : "true") << ")\n"
        << "[--units, -u]             Add units information to JSON file "
           "(default "
        << (default_store_unit_info ? "true" : "false") << ")\n"
        << "[--netdev, -n dev]        Network device to monitor (can be given\n"
        << "                          multiple times; default ALL devices)\n"
        << "[--disable, -d mon]       Disable monitor component\n"
        << "                          (can be given multiple times);\n"
        << "                          all monitors enabled by default\n"
        << "                          Special name '[~]all' sets default "
           "state\n"
        << "[--] prog [arg] ...       Instead of monitoring a PID prmon will\n"
        << "                          execute the given program + args and\n"
        << "                          monitor this (must come after other \n"
        << "                          arguments)\n"
        << "\n"
        << "One of --pid or a child program must be given (but not both)\n"
        << std::endl;
    std::cout << "Monitors available:" << std::endl;
    auto monitors = registry::Registry<Imonitor>::list_registered();
    for (const auto& name : monitors) {
      std::cout << " - " << name << " : "
                << registry::Registry<Imonitor>::get_description(name)
                << std::endl;
    }
    std::cout << std::endl;
    std::cout << "More information: https://github.com/HSF/prmon\n"
              << std::endl;
    return 0;
  }

  int child_args = -1;
  for (int i = 0; i < argc; i++) {
    if (!strcmp(argv[i], "--")) {
      child_args = i + 1;
      break;
    }
  }

  if ((!got_pid && child_args == -1) || (got_pid && child_args > 0)) {
    std::cerr << "One and only one PID or child program is required - ";
    if (got_pid)
      std::cerr << "found both";
    else
      std::cerr << "found none";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  if (got_pid) {
    if (pid < 2) {
      std::cerr << "Bad PID to monitor.\n";
      return 1;
    }
    ProcessMonitor(pid, filename, jsonSummary, interval, store_hw_info,
                   store_unit_info, netdevs, disabled_monitors);
  } else {
    if (child_args == argc) {
      std::cerr << "Found marker for child program to execute, but with no "
                   "program argument.\n";
      return 1;
    }
    pid_t child = fork();
    if (child == 0) {
      execvp(argv[child_args], &argv[child_args]);
    } else if (child > 0) {
      return ProcessMonitor(child, filename, jsonSummary, interval,
                            store_hw_info, store_unit_info, netdevs,
                            disabled_monitors);
    }
  }

  return 0;
}
