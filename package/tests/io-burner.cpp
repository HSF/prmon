// Simple IO burner to simulate workload
// for testing prmon

#include <getopt.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <ratio>
#include <string>
#include <thread>
#include <vector>

#include "io-burner.h"

int io_burn(unsigned long bytes_to_write, std::chrono::nanoseconds nsleep,
            std::chrono::nanoseconds pause) {
  unsigned long bytes_written{0};
  const unsigned int chunk = 1024;
  char write_str[chunk + 1], read_str[chunk + 1];  // Leave room for \0

  for (unsigned int i = 0; i < chunk; ++i) write_str[i] = 'x';
  write_str[chunk] = 0;

  // Implemented with C-style io as tmpfile() does not suffer from
  // race conditions and will not leave any files on disk
  std::FILE* tmp_file = std::tmpfile();
  if (tmp_file == NULL) {
    std::cerr << "Failed to open a temporary file in io-burner" << std::endl;
    return 2;
  }
  while (bytes_written < bytes_to_write) {
    auto res = std::fputs(write_str, tmp_file);
    if (res < 0) {
      std::cerr << "Write problem to temporary file" << std::endl;
      return 2;
    }
    bytes_written += chunk;
    std::this_thread::sleep_for(nsleep);
  }

  std::this_thread::sleep_for(pause);

  std::rewind(tmp_file);
  while (!std::feof(tmp_file)) {
    // fgets will read only up to "chunk" bytes, so this is safe
    auto res = std::fgets(read_str, chunk + 1, tmp_file);
    if (!res and !feof(tmp_file)) {
      std::cerr << "Read problem from temporary file" << std::endl;
      return 2;
    }
    std::this_thread::sleep_for(nsleep);
  }

  std::this_thread::sleep_for(pause);
  fclose(tmp_file);
  return 0;
}

void SignalChildHandler(int /*signal*/) {
  int status;
  waitpid(-1, &status, 0);
  if (status) {
    if (WIFEXITED(status))
      std::cerr << "Warning, monitored child process had non-zero "
      "return value: " << WEXITSTATUS(status) << std::endl;
    else if (WIFSIGNALED(status))
      std::cerr << "Warning, monitored child process exited from signal "
      << WTERMSIG(status) << std::endl;
    else
      std::cerr << "Warning, weird things are happening... "
      << status << std::endl;
  }
}

int main(int argc, char* argv[]) {
  // Default values
  const unsigned long default_io_size = 1;
  const unsigned int default_threads = 1;
  const unsigned int default_procs = 1;
  const float default_usleep = 1.;
  const float default_pause = 1.;

  unsigned long io_size{default_io_size};
  unsigned int threads{default_threads}, procs{default_procs};
  int do_help{0};
  float usleep{default_usleep};
  float pause{default_pause};

  static struct option long_options[] = {
      {"io", required_argument, NULL, 'i'},
      {"threads", required_argument, NULL, 't'},
      {"procs", required_argument, NULL, 'p'},
      {"usleep", required_argument, NULL, 'u'},
      {"pause", required_argument, NULL, 's'},
      {"help", no_argument, NULL, 'h'},
      {0, 0, 0, 0}};

  char c;
  while ((c = getopt_long(argc, argv, "i:t:p:u:s:h", long_options, NULL)) !=
         -1) {
    switch (c) {
      case 'i':
        if (std::stol(optarg) < 1) {
          std::cerr << "IO parameter must be greater than 0 (--help for usage)"
                    << std::endl;
          return 1;
        }
        io_size = std::stol(optarg);
        break;
      case 't':
        if (std::stoi(optarg) < 0) {
          std::cerr << "threads parameter must be greater than or equal to 0 "
                       "(--help for usage)"
                    << std::endl;
          return 1;
        }
        threads = std::stoi(optarg);
        break;
      case 'p':
        if (std::stoi(optarg) < 0) {
          std::cerr << "procs parameter must be greater than or equal to 0 "
                       "(--help for usage)"
                    << std::endl;
          return 1;
        }
        procs = std::stoi(optarg);
        break;
      case 'u':
        if (std::stof(optarg) < 0) {
          std::cerr << "usleep parameter must be greater than or equal to 0 "
                       "(--help for usage)"
                    << std::endl;
          return 1;
        }
        usleep = std::stof(optarg);
        break;
      case 's':
        if (std::stof(optarg) < 0) {
          std::cerr << "pause parameter must be greater than or equal to 0 "
                       "(--help for usage)"
                    << std::endl;
          return 1;
        }
        pause = std::stof(optarg);
        break;
      case 'h':
        do_help = 1;
        break;
      default:
        std::cerr << "Use '--help' for usage" << std::endl;
        return 1;
    }
  }

  if (do_help) {
    std::cout
        << "io-burner is a simple io loading program that can run in multiple "
           "threads\n"
           "and/or processes.\n\n"
           "If both threads and procs are specified then each process runs\n"
           "multiple threads (so the load is threads * procs).\n"
        << std::endl;
    std::cout << "Options:\n"
              << " [--io, -i N]        Number of megabytes to write per "
                 "proc/thread (default "
              << default_io_size << "MB)\n"
              << " [--threads, -t N]   Number of threads to run (default "
              << default_threads << ")\n"
              << " [--procs, -p N]     Number of processes to run (default "
              << default_procs << ")\n"
              << " [--usleep, -u N]    Sleep (in microseconds) between each KB "
                 "of io (default "
              << default_usleep << ")\n"
              << " [--pause, -s N]     Sleep (in seconds) at start, finish and "
                 "between read/write cycles (default "
              << default_pause << ")\n"
              << "                     Default value is recommended to get "
                 "reliable values from /proc\n\n"
              << "If threads or procs is set to 0, the hardware concurrency "
                 "value is used."
              << std::endl;
    return 0;
  }

  // If threads or procs is set to zero, then use the hardware
  // concurrency value
  auto hw = std::thread::hardware_concurrency();
  if (threads == 0) threads = hw;
  if (procs == 0) procs = hw;

  std::cout << "Will write and read " << io_size << "MB * (" << procs
            << " process(es) * " << threads << " thread(s)) with " << usleep
            << " microsecond sleeps" << std::endl;

  unsigned long io_bytes = io_size * std::mega::num;
  std::chrono::nanoseconds nano_sleep(
      static_cast<unsigned long>(usleep * std::milli::den));
  std::chrono::nanoseconds nano_pause(
      static_cast<unsigned long>(pause * std::nano::den));

  // First fork child processes
  pid_t pid = getpid();
  unsigned int children{0};
  if (procs > 1) {
    while (children < procs - 1 && pid != 0) {
      pid = fork();
      ++children;
      std::cout << children << " - " << pid << std::endl;
    }
  }

  // Parent should respond to child exits
  if (pid)
    signal(SIGCHLD, SignalChildHandler);

  // Each process runs the requested number of threads
  std::vector<std::thread> pool;
  for (unsigned int i = 0; i < threads; ++i)
    pool.push_back(std::thread(io_burn, io_bytes, nano_sleep / (pid ? 1 : children), nano_pause ));
  for (auto& th : pool) th.join();

  std::this_thread::sleep_for(nano_pause);

  return 0;
}
