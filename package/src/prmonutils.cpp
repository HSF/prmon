// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

#include "prmonutils.h"

#include <stdlib.h>
#include <sys/stat.h>

#include <cstddef>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>

#include "Imonitor.h"
#include "registry.h"
#include "spdlog/spdlog.h"

namespace prmon {

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
  //
  // This method was absolutely needed on old kernels (SLC6, kernel 2.6
  // vintage). However, we also received reports that SUSE Enterprise
  // Linux 15 SP3 (from HPE) did not have the child PID feature,
  // so we need to keep this old method forever as a fallback in this
  // situation.
  std::vector<pid_t> cpids;
  char smaps_buffer[64];
  snprintf(smaps_buffer, 64, "pstree -l -A -p %ld | tr \\- \\\\n",
           (long)mother_pid);
  FILE *pipe = popen(smaps_buffer, "r");
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
  // Get child process IDs in the modern way, using /proc
  std::vector<pid_t> pid_list{};
  std::deque<pid_t> unprocessed_pids{};

  // Start with the mother PID
  unprocessed_pids.push_back(mother_pid);

  // Now loop over all unprocessed PIDs, querying children
  // and pushing them onto the unprocessed queue, while
  // poping the front onto the final PID list
  while (unprocessed_pids.size() > 0) {
    std::stringstream child_pid_fname{};
    pid_t next_pid;
    child_pid_fname << "/proc/" << unprocessed_pids[0] << "/task/"
                    << unprocessed_pids[0] << "/children" << std::ends;
    std::ifstream proc_children{child_pid_fname.str()};
    while (proc_children) {
      proc_children >> next_pid;
      if (proc_children) unprocessed_pids.push_back(next_pid);
    }
    pid_list.push_back(unprocessed_pids[0]);
    unprocessed_pids.pop_front();
  }
  return pid_list;
}

void SignalCallbackHandler(int /*signal*/) { sigusr1 = true; }

int reap_children() {
  int status;
  int return_code = 0;
  pid_t pid{1};
  while (pid > 0) {
    pid = waitpid((pid_t)-1, &status, WNOHANG);
    if (status && pid > 0) {
      if (WIFEXITED(status)) {
        return_code = WEXITSTATUS(status);
        std::stringstream strm;
        strm << "Child process " << pid
             << " had non-zero return value: " << return_code << std::endl;
        spdlog::warn(strm.str());
      } else if (WIFSIGNALED(status)) {
        std::stringstream strm;
        strm << "Child process " << pid << " exited from signal "
             << WTERMSIG(status) << std::endl;
        spdlog::warn(strm.str());
      } else if (WIFSTOPPED(status)) {
        std::stringstream strm;
        strm << "Child process " << pid << " was stopped by signal"
             << WSTOPSIG(status) << std::endl;
        spdlog::warn(strm.str());
      } else if (WIFCONTINUED(status)) {
        std::stringstream strm;
        strm << "Child process " << pid << " was continued" << std::endl;
        spdlog::warn(strm.str());
      }
    }
  }
  return return_code;
}

// Check if a request to disable a monitor is valid or not
bool valid_monitor_disable(const std::string disable_name) {
  // First check this is not an attempt to disable the wallmon
  if (disable_name == "wallmon") {
    spdlog::error("wallmon monitor cannot be disabled (ignored)");
    return false;
  }
  auto monitors = prmon::get_all_registered();
  for (const auto &monitor_name : monitors) {
    if (monitor_name == disable_name) {
      return true;
    }
  }
  spdlog::error(disable_name + " is an invalid monitor name (ignored)");
  return false;
}

// Look for any monitors we should disable from the environment
void disable_monitors_from_env(std::vector<std::string> &disabled_monitors) {
  char *env_string = std::getenv("PRMON_DISABLE_MONITOR");
  if (!env_string) return;

  // We split this string on commas
  unsigned pos{0}, start{0};
  while (env_string[pos] != '\0') {
    if (env_string[pos] == ',') {
      snip_string_and_test(env_string, start, pos, disabled_monitors);
      start = ++pos;
    } else {
      ++pos;
    }
  }
  snip_string_and_test(env_string, start, pos, disabled_monitors);
}

// Snip a substring from the environment variable c-string
// and test if it's a valid monitor name
void snip_string_and_test(char *env_string, unsigned start, unsigned pos,
                          std::vector<std::string> &disabled_monitors) {
  std::string monitor_name(env_string + start, pos - start);
  if (valid_monitor_disable(monitor_name))
    disabled_monitors.push_back(monitor_name);
}

// Return all registered monitors, regardless of template type
// In practice this means combining monitors with no constructor
// arguments with those (=netmon) which take a list of strings
const std::vector<std::string> get_all_registered() {
  // Standard monitors
  auto registered_monitors = registry::Registry<Imonitor>::list_registered();
  // Special monitors
  auto special_monitors =
      registry::Registry<Imonitor, std::vector<std::string>>::list_registered();
  // Merge
  registered_monitors.insert(registered_monitors.end(),
                             special_monitors.begin(), special_monitors.end());
  return registered_monitors;
}

}  // namespace prmon
