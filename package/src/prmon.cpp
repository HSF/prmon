// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

// PRocess MONitor
// See https://github.com/HSF/prmon

#include "popl.hpp"
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

#include "memmon.h"
#include "prmonVersion.h"
#include "prmonutils.h"
#include "registry.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "utils.h"
#include "wallmon.h"

std::shared_ptr<spdlog::sinks::stdout_color_sink_st> c_sink;
std::shared_ptr<spdlog::sinks::basic_file_sink_st> f_sink;

bool prmon::sigusr1 = false;

int ProcessMonitor(const pid_t mpid, const std::string filename,
                   const std::string json_summary_file, const time_t interval,
                   const bool store_hw_info, const bool store_unit_info,
                   const std::vector<std::string> netdevs,
                   const std::vector<std::string> disabled_monitors,
                   const bool do_fast_memmon) {
  signal(SIGUSR1, prmon::SignalCallbackHandler);

  // This is the vector of all monitoring components
  std::unordered_map<std::string, std::unique_ptr<Imonitor>> monitors{};

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
          monitors[class_name] = std::move(new_monitor_p);
        }
      } else {
        spdlog::error("Registration of monitor " + class_name + " FAILED");
      }
    }
  }

  // The wallclock monitor is always needed as it is used for average stat
  // generation - wallmon cannot be disabled, but this is prechecked in
  // prmon::valid_monitor_disable before we get here
  if (monitors.count("wallmon") != 1) {
    spdlog::error("Failed to initialise mandatory wallclock monitoring class");
    exit(EXIT_FAILURE);
  }

  // Configure the memory monitor for fast monitoring if possible/asked for
  if (do_fast_memmon && monitors.count("memmon")) {
    if (!prmon::smaps_rollup_exists()) {
      spdlog::warn(
          "Fast memory monitoring is requested but the kernel doesn't support "
          "smaps_rollup, using the standard mode.");
    } else {
      auto mem_monitor_p = static_cast<memmon*>(monitors["memmon"].get());
      mem_monitor_p->do_fastmon();
    }
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
          auto wallclock_time = wallclock_monitor_p->get_wallclock_t();
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
            spdlog::error(tmp_json_file.str() + " " + json_snapshot_file.str());
          }
        }
      } catch (const std::ifstream::failure& e) {
        // Serious problem reading one of the status files, usually
        // caused by a child exiting during the poll - just try again
        // next time
        spdlog::warn("prmon ifstream exception: " + std::string(e.what()) +
                     " (ignored)");
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
  if (wallclock_monitor_p->get_wallclock_t() < prmon::mon_value(interval)) {
    spdlog::warn(
        "Monitored process finished before the sampling interval elapsed. "
        "Average statistics will be unreliable. "
        "Consider using --interval <seconds> for short-lived processes.");
  }

  return return_code;
}

int main(int argc, char* argv[]) {
  // Scan argv for "--" separator before popl parsing
  // so that child program arguments are hidden from the option parser
  int child_args = -1;
  int popl_argc = argc;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--")) {
      child_args = i + 1;
      popl_argc = i;
      break;
    }
  }

  // Set up the option parser
  popl::OptionParser op(
      "prmon is a process monitor program that records runtime data\n"
      "from a process and its children, writing time stamped values\n"
      "for resource consumption into a logfile and a JSON summary\n"
      "format when the process exits.");

  auto opt_pid = op.add<popl::Value<int>>(
      "p", "pid", "Monitored process ID", -1);
  auto opt_filename = op.add<popl::Value<std::string>>(
      "f", "filename", "Filename for detailed stats", "prmon.txt");
  auto opt_json_summary = op.add<popl::Value<std::string>>(
      "j", "json-summary", "Filename for JSON summary", "prmon.json");
  auto opt_log_filename = op.add<popl::Value<std::string>>(
      "o", "log-filename", "Filename for logging", "prmon.log");
  auto opt_interval = op.add<popl::Value<unsigned int>>(
      "i", "interval", "Seconds between samples", 30);
  auto opt_suppress = op.add<popl::Switch>(
      "s", "suppress-hw-info", "Disable hardware information");
  auto opt_units = op.add<popl::Switch>(
      "u", "units", "Add units information to JSON file");
  auto opt_netdev = op.add<popl::Value<std::string>>(
      "n", "netdev",
      "Network device to monitor (can be given multiple times;"
      " default ALL devices)");
  auto opt_disable = op.add<popl::Value<std::string>>(
      "d", "disable",
      "Disable monitor component (can be given multiple times;"
      " all monitors enabled by default;"
      " special name '[~]all' sets default state)");
  auto opt_level = op.add<popl::Value<std::string>>(
      "l", "level",
      "Set logging level (e.g. 'info' or 'mon:debug';"
      " valid levels: trace, debug, info, warn, error, critical)");
  auto opt_fast_memmon = op.add<popl::Switch>(
      "m", "fast-memmon",
      "Do fast memory monitoring using smaps_rollup");
  auto opt_help = op.add<popl::Switch>(
      "h", "help", "Show this help message");

  // Parse only the prmon options (not child args)
  op.parse(popl_argc, argv);

  // Handle help request
  if (opt_help->is_set()) {
    std::cout << op << std::endl;
    std::cout
        << "[--] prog [arg] ...       Instead of monitoring a PID prmon "
           "will\n"
        << "                          execute the given program + args and\n"
        << "                          monitor this (must come after other\n"
        << "                          arguments)\n"
        << "\n"
        << "One of --pid or a child program must be given (but not both)\n"
        << std::endl;
    std::cout << "Monitors available:" << std::endl;
    auto monitors = prmon::get_all_registered();
    for (const auto& name : monitors) {
      std::cout
          << " - " << name << " : "
          << (name == "netmon"
                  ? registry::Registry<Imonitor, std::vector<std::string>>::
                        get_description(name)
                  : registry::Registry<Imonitor>::get_description(name))
          << std::endl;
    }
    std::cout << std::endl;
    std::cout << "More information: https://github.com/HSF/prmon\n"
              << std::endl;
    return 0;
  }

  // Extract values from parsed options
  pid_t pid = opt_pid->value();
  bool got_pid = opt_pid->is_set();
  std::string filename = opt_filename->value();
  std::string jsonSummary = opt_json_summary->value();
  std::string logFileName = opt_log_filename->value();
  unsigned int interval = opt_interval->value();
  bool store_hw_info = !opt_suppress->is_set();
  bool store_unit_info = opt_units->is_set();
  bool do_fast_memmon = opt_fast_memmon->is_set();

  // Collect repeatable options
  std::vector<std::string> netdevs{};
  for (size_t i = 0; i < opt_netdev->count(); ++i) {
    netdevs.push_back(opt_netdev->value(i));
  }

  std::vector<std::string> disabled_monitors{};
  for (size_t i = 0; i < opt_disable->count(); ++i) {
    if (prmon::valid_monitor_disable(opt_disable->value(i)))
      disabled_monitors.push_back(opt_disable->value(i));
  }

  for (size_t i = 0; i < opt_level->count(); ++i) {
    processLevel(opt_level->value(i));
  }

  // Get additional configuration from the environment
  prmon::disable_monitors_from_env(disabled_monitors);

  if (invalid_level_option) {
    return EXIT_FAILURE;
  }

  // Set up the global logger
  c_sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
  f_sink =
      std::make_shared<spdlog::sinks::basic_file_sink_st>(logFileName, true);
  spdlog::sinks_init_list s_list = {c_sink, f_sink};
  auto logger =
      std::make_shared<spdlog::logger>("prmon", s_list.begin(), s_list.end());
  logger->set_level(global_logging_level);
  logger->flush_on(global_logging_level);
  spdlog::set_default_logger(logger);

  if ((!got_pid && child_args == -1) || (got_pid && child_args > 0)) {
    std::stringstream strm;
    strm << "One and only one PID or child program is required - ";
    if (got_pid)
      strm << "found both";
    else
      strm << "found none";
    spdlog::error(strm.str());
    return EXIT_FAILURE;
  }

  if (got_pid) {
    if (pid < 2) {
      spdlog::error("Bad PID to monitor.");
      return 1;
    }
    ProcessMonitor(pid, filename, jsonSummary, interval, store_hw_info,
                   store_unit_info, netdevs, disabled_monitors, do_fast_memmon);
  } else {
    if (child_args == argc) {
      spdlog::error(
          "Found marker for child program to execute, but with no "
          "program argument.");
      return 1;
    }
    pid_t child = fork();
    if (child == 0) {
      execvp(argv[child_args], &argv[child_args]);
    } else if (child > 0) {
      return ProcessMonitor(child, filename, jsonSummary, interval,
                            store_hw_info, store_unit_info, netdevs,
                            disabled_monitors, do_fast_memmon);
    }
  }

  return 0;
}
