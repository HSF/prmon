//  Copyright (C) 2018, CERN

// PRocess MONitor
// See https://github.com/HSF/prmon

#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <condition_variable>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>
#include <iomanip>

#include <nlohmann/json.hpp>

#include "cpumon.h"
#include "iomon.h"
#include "memmon.h"
#include "netmon.h"
#include "pidutils.h"
#include "prmon.h"
#include "wallmon.h"
#include "countmon.h"

bool sigusr1 = false;

void SignalCallbackHandler(int /*signal*/) {
  sigusr1 = true;
}

void SignalChildHandler(int /*signal*/) {
  int status;
  pid_t pid{1};
  while (pid > 0) {
    pid = waitpid((pid_t)-1, &status, WNOHANG);
    if (status && pid > 0) {
      if (WIFEXITED(status))
        std::clog << "Child process " << pid
                  << " had non-zero return value: " << WEXITSTATUS(status)
                  << std::endl;
      else if (WIFSIGNALED(status))
        std::clog << "Child process " << pid << " exited from signal "
                  << WTERMSIG(status) << std::endl;
      else if (WIFSTOPPED(status))
        std::clog << "Child process " << pid << " was stopped by signal"
                  << WSTOPSIG(status) << std::endl;
      else if (WIFCONTINUED(status))
        std::clog << "Child process " << pid << " was continued" << std::endl;
    }
  }
}

int MemoryMonitor(const pid_t mpid, const std::string filename,
                  const std::string jsonSummary, const unsigned int interval,
                  const std::vector<std::string> netdevs) {
  signal(SIGUSR1, SignalCallbackHandler);
  signal(SIGCHLD, SignalChildHandler);

  // This is the vector of all monitoring components
  std::vector<Imonitor*> monitors{};

  // Number of processes and threads monitoring
  countmon count_monitor{};
  monitors.push_back(&count_monitor);

  // Wall clock monitoring
  wallmon wall_monitor{};
  monitors.push_back(&wall_monitor);

  // CPU monitoring
  cpumon cpu_monitor{};
  monitors.push_back(&cpu_monitor);

  // Memory monitoring
  memmon mem_monitor{};
  monitors.push_back(&mem_monitor);

  // IO monitoring
  iomon io_monitor{};
  monitors.push_back(&io_monitor);

  // Network monitoring
  netmon network_monitor{netdevs};
  monitors.push_back(&network_monitor);

  int iteration = 0;
  time_t lastIteration = time(0) - interval;
  time_t currentTime;

  // Open iteration output file
  std::ofstream file;
  file.open(filename);
  file << "Time";
  for (const auto monitor : monitors) {
    for (const auto& stat : monitor->get_text_stats())
      file << "\t" << stat.first;
  }
  file << std::endl;

  // Construct string representing JSON structure
  std::stringstream json{};
  json << "{\"Max\":  {";
  bool started = false;
  for (const auto monitor : monitors) {
    for (const auto& stat : monitor->get_json_total_stats()) {
      if (started) json << ", ";
      else started = true;
      json << "\"" << stat.first << "\" : 0";
    }
  }
  json << "}, \"Avg\":  {";
  started = false;
  for (const auto monitor : monitors) {
    for (const auto& stat : monitor->get_json_average_stats(1)) {
      if (started) json << ", ";
      else started = true;
      json << "\"" << stat.first << "\" : 0";
    }
  }
  json << "}}" << std::ends;

  std::stringstream tmpFile;
  tmpFile << jsonSummary << "_tmp";
  std::stringstream newFile;
  newFile << jsonSummary << "_snapshot";

  // Parse JSON explicitly
  auto d = nlohmann::json::parse(json);

  // See if the kernel is new enough to have /proc/PID/task/PID/children
  bool modern_kernel = kernel_proc_pid_test(mpid);

  // Monitoring loop until process exits
  bool wroteFile = false;
  std::vector<pid_t> cpids{};
  while (kill(mpid, 0) == 0 && sigusr1 == false) {
    if (time(0) - lastIteration > interval) {
      iteration++;
      // Reset lastIteration
      lastIteration = time(0);

      if (modern_kernel)
        cpids = offspring_pids(mpid);
      else
        cpids = pstree_pids(mpid);

      try {
        for (const auto monitor : monitors) monitor->update_stats(cpids);

        currentTime = time(0);
        file << currentTime;
        for (const auto monitor : monitors) {
          for (const auto& stat : monitor->get_text_stats())
            file << "\t" << stat.second;
        }
        file << std::endl;

        // Reset buffer
        d.clear();

        // Create JSON realtime summary
        for (const auto monitor : monitors)
          for (const auto& stat : monitor->get_json_total_stats())
            d["Max"][(stat.first).c_str()] = stat.second;
        for (const auto monitor : monitors)
          for (const auto& stat : monitor->get_json_average_stats(
                   wall_monitor.get_wallclock_clock_t()))
            d["Avg"][(stat.first).c_str()] = stat.second;

        // Write JSON realtime summary to a temporary file
        std::ofstream json_out(tmpFile.str());
        json_out << std::setw(4) << d << std::endl;
        json_out.close();
        wroteFile = true;

        // Move temporary file to new file
        if (wroteFile) {
          if (rename(tmpFile.str().c_str(), newFile.str().c_str()) != 0) {
            perror("rename fails");
            std::cerr << tmpFile.str() << " " << newFile.str() << "\n";
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
  }
  file.close();

  // Cleanup
  if (remove(newFile.str().c_str()) != 0 && iteration > 0)
    perror("remove fails");

  // Write final JSON summary file
  file.open(jsonSummary);
  file << std::setw(4) << d << std::endl;
  file.close();

  return 0;
}

int main(int argc, char* argv[]) {
  // Set defaults
  const char* default_filename = "prmon.txt";
  const char* default_json_summary = "prmon.json";
  const unsigned int default_interval = 30;

  pid_t pid = -1;
  bool got_pid = false;
  std::string filename{default_filename};
  std::string jsonSummary{default_json_summary};
  std::vector<std::string> netdevs{};
  unsigned int interval{default_interval};
  int do_help{0};

  static struct option long_options[] = {
      {"pid", required_argument, NULL, 'p'},
      {"filename", required_argument, NULL, 'f'},
      {"json-summary", required_argument, NULL, 'j'},
      {"interval", required_argument, NULL, 'i'},
      {"netdev", required_argument, NULL, 'n'},
      {"help", no_argument, NULL, 'h'},
      {0, 0, 0, 0}};

  char c;
  while ((c = getopt_long(argc, argv, "p:f:j:i:n:h", long_options, NULL)) !=
         -1) {
    switch (c) {
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
      case 'n':
        netdevs.push_back(optarg);
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
        << "[--netdev, -n dev]        Network device to monitor (can be given\n"
        << "                          multiple times; default ALL devices)\n"
        << "[--] prog [arg] ...       Instead of monitoring a PID prmon will\n"
        << "                          execute the given program + args and\n"
        << "                          monitor this (must come after other \n"
        << "                          arguments)\n"
        << "\n"
        << "One of --pid or a child program must be given (but not both)\n"
        << std::endl;
    return 0;
  }

  int child_args = -1;
  for (int i = 0; i < argc; i++ ) {
    if (!strcmp(argv[i], "--")) {
      child_args = i+1;
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
    return 0;
  }

  if (got_pid) {
    if (pid < 2) {
      std::cerr << "Bad PID to monitor.\n";
      return 1;
    }
    MemoryMonitor(pid, filename, jsonSummary, interval, netdevs);
  } else {
    if (child_args == argc) {
      std::cerr << "Found marker for child program to execute, but with no program argument.\n";
      return 1;
    }
    pid_t child = fork();
    if( child == 0 ) {
      execvp(argv[child_args],&argv[child_args]);
    } else if ( child > 0 ) {
      MemoryMonitor(child, filename, jsonSummary, interval, netdevs);
    }
  }

  return 0;
}
