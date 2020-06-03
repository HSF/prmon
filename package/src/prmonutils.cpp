// Copyright (C) CERN, 2018

// A few utilities handling PID operations

#include <sys/stat.h>
#include <stdlib.h>

#include <cstddef>
#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>

#include "prmonutils.h"

bool kernel_proc_pid_test(const pid_t pid) {
  // Return true if the kernel has child PIDs
  // accessible via /proc
  std::stringstream pid_fname{};
  pid_fname << "/proc/" << pid << "/task/" << pid << "/children" << std::ends;
  struct stat stat_test;
  if (stat(pid_fname.str().c_str(), &stat_test)) return false;
  return true;
}

std::vector<pid_t> pstree_pids(const pid_t mother_pid) {
  // This is the old style method to get the list
  // of PIDs, which uses pstree
  std::vector<pid_t> cpids;
  char smaps_buffer[64];
  snprintf(smaps_buffer, 64, "pstree -A -p %ld | tr \\- \\\\n",
           (long)mother_pid);
  FILE* pipe = popen(smaps_buffer, "r");
  if (pipe == 0) return cpids;

  char buffer[256];
  std::string result = "";
  while (!feof(pipe)) {
    if (fgets(buffer, 256, pipe) != NULL) {
      result += buffer;
      int pos(0);
      while (pos < 256 && buffer[pos] != '\n' && buffer[pos] != '(') {
        pos++;
      }
      if (pos < 256 && buffer[pos] == '(' && pos > 1 &&
          buffer[pos - 1] != '}') {
        pos++;
        pid_t pt(0);
        while (pos < 256 && buffer[pos] != '\n' && buffer[pos] != ')') {
          pt = 10 * pt + buffer[pos] - '0';
          pos++;
        }
        cpids.push_back(pt);
      }
    }
  }
  pclose(pipe);
  return cpids;
}

std::vector<pid_t> offspring_pids(const pid_t mother_pid) {
  // Get child process IDs in the new way, using
  // /proc
  std::vector<pid_t> pid_list{};
  std::deque<pid_t> unprocessed_pids{};

  // Start with the mother PID
  unprocessed_pids.push_back(mother_pid);

  // Now loop over all unprocessed PIDs, querying children
  // and pushing them onto the unprocessed queue, while
  // poping the front onto the final PID list
  while(unprocessed_pids.size() > 0) {
    std::stringstream child_pid_fname{};
    pid_t next_pid;
    child_pid_fname << "/proc/" << unprocessed_pids[0] << "/task/"
                    << unprocessed_pids[0] << "/children" << std::ends;
    std::ifstream proc_children{child_pid_fname.str()};
    while (proc_children) {
      proc_children >> next_pid;
      if (proc_children)
        unprocessed_pids.push_back(next_pid);
    }
    pid_list.push_back(unprocessed_pids[0]);
    unprocessed_pids.pop_front();
  }
  return pid_list;
}



const monitor_switch_t parse_monitor_switches(const std::vector<std::string> monitor_args) {
  // Parse a vector of monitor switch strings and decide on the
  // state of each monitor. Each switch string needs to be split
  // on ",", then passed to the "decision" function.

  monitor_switch_t monitor_switches{};
  for (const auto& mon_opt : monitor_args) {
    std::string::size_type prev_pos = 0, pos = 0;
    while((pos = mon_opt.find(',', pos)) != std::string::npos) {
      // Get substring and pass to interpret function
      monitor_switches.insert(monitor_switch_state(mon_opt.substr(prev_pos, pos-prev_pos)));
      prev_pos = ++pos;
    }
    // Last word
    monitor_switches.insert(monitor_switch_state(mon_opt.substr(prev_pos, pos-prev_pos)));
  }
  return monitor_switches;
}

const std::pair<std::string, bool> monitor_switch_state(const std::string monitor) {
  // Simple parser to see if the monitor switch has a ~ (meaning off)
  if (monitor[0] == '~') {
    std::pair<std::string, bool> ret{monitor.substr(1, monitor.size()), false};
    return ret;
  }
  std::pair<std::string, bool> ret{monitor, true};
  return ret;
}
