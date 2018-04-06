/*
  Copyright (C) 2018, CERN
*/

#include <algorithm>
#include <dirent.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <condition_variable>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "prmon.h"

using namespace rapidjson;

int ReadProcs(const pid_t mother_pid, unsigned long values[4],
              unsigned long long valuesIO[4], unsigned long long valuesCPU[4],
              const bool verbose) {
  // Get child process IDs
  std::vector<pid_t> cpids;
  char smaps_buffer[64];
  char io_buffer[64];
  char stat_buffer[64];
  snprintf(smaps_buffer, 64, "pstree -A -p %ld | tr \\- \\\\n",
           (long)mother_pid);
  FILE* pipe = popen(smaps_buffer, "r");
  if (pipe == 0) {
    if (verbose)
      std::cerr << "MemoryMonitor: unable to open pstree pipe!" << std::endl;
    return 1;
  }

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

  unsigned long tsize(0);
  unsigned long trss(0);
  unsigned long tpss(0);
  unsigned long tswap(0);

  unsigned long long trchar(0);
  unsigned long long twchar(0);
  unsigned long long trbyte(0);
  unsigned long long twbyte(0);

  unsigned long long utime(0);
  unsigned long long stime(0);
  unsigned long long cutime(0);
  unsigned long long cstime(0);

  std::vector<std::string> openFails;

  char sbuffer[2048], *tsbuffer;
  for (std::vector<pid_t>::const_iterator it = cpids.begin(); it != cpids.end();
       ++it) {
    snprintf(smaps_buffer, 64, "/proc/%lu/smaps", (unsigned long)*it);

    FILE* file = fopen(smaps_buffer, "r");
    if (file == 0) {
      openFails.push_back(std::string(smaps_buffer));
    } else {
      while (fgets(buffer, 256, file)) {
        if (sscanf(buffer, "Size: %80lu kB", &tsize) == 1) values[0] += tsize;
        if (sscanf(buffer, "Pss: %80lu kB", &tpss) == 1) values[1] += tpss;
        if (sscanf(buffer, "Rss: %80lu kB", &trss) == 1) values[2] += trss;
        if (sscanf(buffer, "Swap: %80lu kB", &tswap) == 1) values[3] += tswap;
      }
      fclose(file);
    }

    snprintf(io_buffer, 64, "/proc/%llu/io", (unsigned long long)*it);

    FILE* file2 = fopen(io_buffer, "r");
    if (file2 == 0) {
      openFails.push_back(std::string(io_buffer));
    } else {
      while (fgets(buffer, 256, file2)) {
        if (sscanf(buffer, "rchar: %80llu", &trchar) == 1)
          valuesIO[0] += trchar;
        if (sscanf(buffer, "wchar: %80llu", &twchar) == 1)
          valuesIO[1] += twchar;
        if (sscanf(buffer, "read_bytes: %80llu", &trbyte) == 1)
          valuesIO[2] += trbyte;
        if (sscanf(buffer, "write_bytes: %80llu", &twbyte) == 1)
          valuesIO[3] += twbyte;
      }
      fclose(file2);
    }

    snprintf(stat_buffer, 64, "/proc/%llu/stat", (unsigned long long)*it);

    FILE* file3 = fopen(stat_buffer, "r");

    if (file3 == 0) {
      openFails.push_back(std::string(stat_buffer));
    } else {
      while (fgets(sbuffer, 2048, file3)) {
        tsbuffer = strchr(sbuffer, ')');
        if (sscanf(tsbuffer + 2,
                   "%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %80llu %80llu "
                   "%80llu %80llu",
                   &utime, &stime, &cutime, &cstime)) {
          valuesCPU[0] += utime;
          valuesCPU[1] += stime;
          valuesCPU[2] += cutime;
          valuesCPU[3] += cstime;
        }
      }
      fclose(file3);
    }
  }
  if (openFails.size() > 3 && verbose) {
    std::cerr << "ProcMonitor: too many failures in opening smaps, io, and "
                 "stat files!"
              << std::endl;
    return 1;
  }

  return 0;
}

// This is all a bit yuk, using C style directory
// parsing. From C++17 we should use the filesystem
// library, but only when we decide it's reasonable
// to no longer support older compilers.
std::vector<std::string> get_network_device_names() {
  std::vector<std::string> devices{};
  DIR* d;
  struct dirent* dir;
  const char* netdir = "/sys/class/net";
  d = opendir(netdir);
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (!(!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")))
        devices.push_back(dir->d_name);
    }
    closedir(d);
  } else {
    std::cerr << "Failed to open " << netdir
              << " to get list of network devices. "
              << "No network data will be available" << std::endl;
  }
  return devices;
}

int read_net_stats(
    const std::vector<std::string> devices,
    std::unordered_map<std::string, unsigned long long>& values) {
  unsigned long long value_read{};
  std::string filename{};

  for (auto& element : values) values[element.first] = 0;

  for (const auto& device : devices) {
    for (auto& element : values) {
      filename = "/sys/class/net/" + device + "/statistics/" + element.first;
      std::ifstream input{filename, std::ios::binary};
      input >> value_read;
      values[element.first] += value_read;
    }
  }
  return 0;
}

std::condition_variable cv;
std::mutex cv_m;
bool sigusr1 = false;

void SignalCallbackHandler(int /*signal*/) {
  std::lock_guard<std::mutex> l(cv_m);
  sigusr1 = true;
  cv.notify_one();
}

int MemoryMonitor(const pid_t mpid, const std::string filename,
                  const std::string jsonSummary, const unsigned int interval,
                  const std::vector<std::string> netdevs) {
  signal(SIGUSR1, SignalCallbackHandler);

  unsigned long values[4] = {0, 0, 0, 0};
  unsigned long maxValues[4] = {0, 0, 0, 0};
  unsigned long avgValues[4] = {0, 0, 0, 0};

  unsigned long long valuesIO[4] = {0, 0, 0, 0};
  unsigned long long maxValuesIO[4] = {0, 0, 0, 0};
  unsigned long long avgValuesIO[4] = {0, 0, 0, 0};

  unsigned long long valuesCPU[4] = {0, 0, 0, 0};
  unsigned long long maxValuesCPU[4] = {0, 0, 0, 0};

  const std::vector<std::string> netstats{"rx_bytes", "rx_packets", "tx_bytes",
                                          "tx_packets"};
  std::unordered_map<std::string, unsigned long long> values_netstats_start{},
      values_netstats{}, avg_values_netstats{};
  std::vector<std::string> devices{};
  for (const auto& stat : netstats) {
    values_netstats_start.insert({stat, 0});
    values_netstats.insert({stat, 0});
    avg_values_netstats.insert({stat, 0});
  }
  if (netdevs.size() > 0) {
    devices = netdevs;
  } else {
    devices = get_network_device_names();
  }
  read_net_stats(devices, values_netstats_start);

  int iteration = 0;
  time_t lastIteration = time(0) - interval;
  time_t startTime;
  time_t currentTime;

  // Open iteration output file
  std::ofstream file;
  file.open(filename);
  file << "Time\tVMEM\tPSS\tRSS\tSwap\trchar\twchar\trbytes\twbytes\tutime\tsti"
          "me\tcutime\tcstime\twtime";
  for (const auto& stat : netstats) file << "\t" << stat;
  file << std::endl;

  // Construct string representing JSON structure
  std::stringstream json{};
  json << "{\"Max\":  {\"maxVMEM\": 0, \"maxPSS\": 0,\"maxRSS\": 0, "
          "\"maxSwap\": 0, \"totRCHAR\": 0, \"totWCHAR\": 0,\"totRBYTES\": 0, "
          "\"totWBYTES\": 0, \"totUTIME\" : 0, \"totSTIME\" : 0, \"totCUTIME\" "
          ": 0, \"totCSTIME\" : 0, \"totWTIME\" : 0";
  for (const auto& stat : netstats)
    json << ", \""
         << "tot_" << stat << "\" : 0";
  json << "}, \"Avg\":  {\"avgVMEM\": 0, \"avgPSS\": "
          "0,\"avgRSS\": 0, \"avgSwap\": 0, \"rateRCHAR\": 0, \"rateWCHAR\": "
          "0,\"rateRBYTES\": 0, \"rateWBYTES\": 0";
  for (const auto& stat : netstats)
    json << ", \""
         << "avg_" << stat << "\" : 0";
  json << "}}" << std::ends;

  Document d;
  d.Parse(json.str().c_str());
  std::ofstream file2;  // for realtime json dict
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);

  std::stringstream tmpFile;
  tmpFile << jsonSummary << "_tmp";
  std::stringstream newFile;
  newFile << jsonSummary << "_snapshot";

  int tmp = 0;
  Value& v1 = d["Max"];
  Value& v2 = d["Avg"];

  // Get mother process start time
  char stat_buffer[64], sbuffer[2048], *tsbuffer;

  snprintf(stat_buffer, 64, "/proc/%llu/stat", (unsigned long long)mpid);

  FILE* stat_file = fopen(stat_buffer, "r");
  unsigned long long thispid(0);
  unsigned long long starttime(0);

  if (stat_file == 0) {
    std::cerr << "Cannot open stat buffer for mother pid to get the starttime"
              << std::endl;
  } else {
    while (fgets(sbuffer, 2048, stat_file)) {
      if (sscanf(sbuffer, "%80llu", &thispid)) {
        if (thispid != (unsigned long long)mpid) continue;
        tsbuffer = strchr(sbuffer, ')');
        if (sscanf(tsbuffer + 2,
                   "%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*d "
                   "%*d %*d %*d %*d %*d %80llu",
                   &starttime)) {
          break;
        }
      }
    }
    fclose(stat_file);
  }

  // Get system uptime
  snprintf(stat_buffer, 64, "/proc/uptime");

  stat_file = fopen(stat_buffer, "r");
  float uptime(0.);

  if (stat_file == 0) {
    std::cerr << "Cannot open stat buffer to get the uptime" << std::endl;
  } else {
    while (fgets(sbuffer, 2048, stat_file)) {
      if (sscanf(sbuffer, "%f", &uptime)) {
        break;
      }
    }
    fclose(stat_file);
  }

  long clock_ticks = sysconf(_SC_CLK_TCK);
  float inv_clock_ticks = 1. / clock_ticks;

  // Start time
  startTime = time(0);
  startTime += static_cast<time_t>(float(starttime) * inv_clock_ticks - uptime);

  // Monitoring loop until process exits
  while (kill(mpid, 0) == 0 && sigusr1 == false) {
    bool wroteFile = false;
    if (time(0) - lastIteration > interval) {
      iteration = iteration + 1;
      ReadProcs(mpid, values, valuesIO, valuesCPU);
      read_net_stats(devices, values_netstats);

      currentTime = time(0);
      file << currentTime << "\t" << values[0] << "\t" << values[1] << "\t"
           << values[2] << "\t" << values[3] << "\t" << valuesIO[0] << "\t"
           << valuesIO[1] << "\t" << valuesIO[2] << "\t" << valuesIO[3] << "\t"
           << valuesCPU[0] * inv_clock_ticks << "\t"
           << valuesCPU[1] * inv_clock_ticks << "\t"
           << valuesCPU[2] * inv_clock_ticks << "\t"
           << valuesCPU[3] * inv_clock_ticks << "\t"
           << difftime(currentTime, startTime);
      for (const auto& stat : netstats)
        file << "\t" << values_netstats[stat] - values_netstats_start[stat];
      file << std::endl;

      // Compute statistics
      for (int i = 0; i < 4; i++) {
        avgValues[i] = avgValues[i] + values[i];
        if (values[i] > maxValues[i]) maxValues[i] = values[i];
        lastIteration = time(0);

        if (valuesIO[i] > maxValuesIO[i]) maxValuesIO[i] = valuesIO[i];

        avgValuesIO[i] = (unsigned long long)maxValuesIO[i] /
                         difftime(currentTime, startTime);

        if (valuesCPU[i] > maxValuesCPU[i]) maxValuesCPU[i] = valuesCPU[i];
      }
      for (const auto& stat : netstats) {
        avg_values_netstats[stat] =
            static_cast<float>(values_netstats[stat] -
                               values_netstats_start[stat]) /
            difftime(currentTime, startTime);
      }

      // Reset buffer
      buffer.Clear();
      writer.Reset(buffer);

      for (int i = 0; i < 4; i++) {
        values[i] = 0;
        valuesIO[i] = 0;
        valuesCPU[i] = 0;
      }

      // Create JSON realtime summary
      for (std::pair<Value::MemberIterator, Value::MemberIterator> i =
               std::make_pair(v1.MemberBegin(), v2.MemberBegin());
           i.first != v1.MemberEnd() && i.second != v2.MemberEnd();
           ++i.first, ++i.second) {
        if (tmp < 4) {
          i.first->value.SetUint64(maxValues[tmp]);
          i.second->value.SetUint64(avgValues[tmp] / iteration);
        } else if (tmp < 8) {
          i.first->value.SetUint64(maxValuesIO[tmp - 4]);
          i.second->value.SetUint64(avgValuesIO[tmp - 4]);
        }
        tmp += 1;
      }
      tmp = 0;

      // Total CPU measurements
      for (Value::MemberIterator it = v1.MemberBegin() + 8;
           it != v1.MemberEnd(); ++it) {
        if (tmp < 4) {
          it->value.SetFloat(maxValuesCPU[tmp] * inv_clock_ticks);
        } else {
          it->value.SetFloat(difftime(currentTime, startTime));
        }
        tmp += 1;
      }
      tmp = 0;

      // Network stats
      for (const auto& stat : netstats) {
        v1[("tot_" + stat).c_str()].SetUint64(values_netstats[stat] -
                                              values_netstats_start[stat]);
        v2[("avg_" + stat).c_str()].SetFloat(avg_values_netstats[stat]);
      }

      // Write JSON realtime summary to a temporary file (to avoid race
      // conditions with pilot trying to read from file at the same time)
      d.Accept(writer);
      file2.open(tmpFile.str());
      file2 << buffer.GetString() << std::endl;
      file2.close();
      wroteFile = true;
    }

    // Move temporary file to new file
    if (wroteFile) {
      if (rename(tmpFile.str().c_str(), newFile.str().c_str()) != 0) {
        perror("rename fails");
        std::cerr << tmpFile.str() << " " << newFile.str() << "\n";
      }
    }

    std::unique_lock<std::mutex> lock(cv_m);
    cv.wait_for(lock, std::chrono::seconds(1));
  }
  file.close();

  // Cleanup
  if (remove(newFile.str().c_str()) != 0) perror("remove fails");

  // Write final JSON summary file
  file.open(jsonSummary);
  file << buffer.GetString() << std::endl;
  file.close();

  return 0;
}

int main(int argc, char* argv[]) {
  // Set defaults
  const char* default_filename = "prmon.txt";
  const char* default_json_summary = "prmon.json";
  const unsigned int default_interval = 1;

  pid_t pid = -1;
  std::string filename{default_filename};
  std::string jsonSummary{default_json_summary};
  std::vector<std::string> netdevs{};
  std::string invokeargs{""};
  unsigned int interval{default_interval};
  int do_help{0};

  static struct option long_options[] = {
      {"pid", required_argument, NULL, 'p'},
      {"filename", required_argument, NULL, 'f'},
      {"json-summary", required_argument, NULL, 'j'},
      {"interval", required_argument, NULL, 'i'},
      {"netdev", required_argument, NULL, 'n'},
      {"help", no_argument, NULL, 'h'},
      {"args", required_argument, NULL, 'a'},
      {0, 0, 0, 0}};

  char c;
  while ((c = getopt_long(argc, argv, "p:f:j:i:n:a:h", long_options, NULL)) !=
         -1) {
    switch (c) {
      case 'p':
        pid = std::stoi(optarg);
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
      case 'a':
        invokeargs = optarg;
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
        << "--pid, -p PID             Monitored process ID\n"
        << "[--filename, -f FILE]     Filename for detailed stats (default "
        << default_filename << ")\n"
        << "[--json-summary, -j FILE] Filename for JSON summary (default "
        << default_json_summary << ")\n"
        << "[--interval, -i TIME]     Seconds between samples (default "
        << default_interval << ")\n"
        << "[--netdev, -n dev]        Network device to monitor (can be given\n"
        << "                          multiple times; default ALL devices)\n"
        << std::endl;
    return 0;
  }

  if (pid < 2) {
    if( invokeargs.empty() ) {
      std::cerr << "Bad PID to monitor.\n";
      return 1;
    }

    pid_t child = fork();
    //int child_status;

    if( child == 0 ) {
      // Child
      char* cargs = strdup(invokeargs.c_str());
      char* ctoken = strtok(cargs," ");
      char* program[100] = { NULL };
      unsigned int counter = 0;
      while(ctoken != NULL) {
        program[counter] = ctoken;
        ctoken = strtok(NULL," ");
        counter++;
      }
      execv(program[0],program);
    } else if ( child > 0 ) {
      // Parent
      std::cout << "Listening to " << child << std::endl;
      MemoryMonitor(child, filename, jsonSummary, interval, netdevs);
      //waitpid(child, &child_status, 0);
    }
  } else {
    MemoryMonitor(pid, filename, jsonSummary, interval, netdevs);
  }

  return 0;
}
