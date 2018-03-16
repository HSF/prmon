// Simple CPU burner to simulate workload
// for testing prmon

#include <cmath>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <ratio>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <unistd.h>
#include <getopt.h>

#include "io-burner.h"

int io_burn(unsigned long bytes_to_write, unsigned int nsleep=0) {
  unsigned long bytes_written{0};
  std::string write_str{};
  write_str.insert(0, 1024, 'x');
  const size_t write_len = write_str.length();
  char read_str[write_len];

  std::FILE* tmp_file = std::tmpfile();
  if (tmp_file == NULL) {
    std::cerr << "Failed to open a temporary file in io-burner" << std::endl;
    return 2;
  }
  while (bytes_written < bytes_to_write) {
    std::fputs(write_str.c_str(), tmp_file);
    bytes_written += write_len;
    if (nsleep) std::this_thread::sleep_for(std::chrono::nanoseconds(nsleep));
  }

  std::rewind(tmp_file);
  while (!std::feof(tmp_file)) {
    std::fgets(read_str, write_len, tmp_file);
    if (nsleep) std::this_thread::sleep_for(std::chrono::nanoseconds(nsleep));
  }

  fclose(tmp_file);
  return 0;
}

int main(int argc, char *argv[]) {
  // Default values
  const unsigned long default_io_size = 1;
  const unsigned int default_threads = 1;
  const unsigned int default_procs = 1;
  const float default_usleep = 1;

  unsigned long io_size{default_io_size};
  unsigned int threads{default_threads}, procs{default_procs};
  int do_help{0};
  float usleep = default_usleep;

  static struct option long_options[] = {
      {"io", required_argument, NULL, 'i'},
      {"threads", required_argument, NULL, 't'},
      {"procs", required_argument, NULL, 'p'},
      {"usleep", required_argument, NULL, 'u'},
      {"help", no_argument, NULL, 'h'},
      {0, 0, 0, 0}
  };

  char c;
  while ((c = getopt_long(argc, argv, "i:t:p:u:h", long_options, NULL)) != -1) {
    switch (c) {
    case 'i':
      io_size = std::stol(optarg);
      break;
    case 't':
      threads = std::stoi(optarg);
      break;
    case 'p':
      procs = std::stoi(optarg);
      break;
    case 'u':
      usleep = std::stof(optarg);
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
    std::cout <<
        "io-burner is a simple io loading program that can run in multiple threads\n"
        "and/or processes.\n\n"
        "If both threads and procs are specified then each process runs\n"
        "multiple threads (so the load is threads * procs).\n" << std::endl;
    std::cout << "Options:\n"
        << " [--io, -i N]        Number of megabytes to write per proc/thread (default " << default_io_size << "MB)\n"
        << " [--threads, -t N]   Number of threads to run (default " << default_threads << "\n"
        << " [--procs, -p N]     Number of processes to run (default " << default_procs << ")\n"
        << " [--usleep, -u 1]    Sleep (in microseconds) between each KB of io (default " << default_usleep << ")\n\n"
        << "If threads or procs is set to 0, the hardware concurrency value is used." << std::endl;
    return 0;
  }

  // If threads or procs is set to zero, then use the hardware
  // concurrency value
  auto hw = std::thread::hardware_concurrency();
  if (threads==0) threads=hw;
  if (procs==0) procs=hw;

  std::cout << "Will write and read " << io_size << "MB * (" << procs <<
      " process(es) * " << threads << " thread(s)) with " << usleep << "microsecond sleeps" << std::endl;

  // First fork child processes
  if (procs > 1) {
    unsigned int children{0};
    pid_t pid{0};
    while (children < procs-1 && pid == 0) {
      pid = fork();
      ++children;
    }
  }

  // Each process runs the requested number of threads
  std::vector<std::thread> pool;
  for (int i=0; i<threads; ++i)
    pool.push_back(std::thread(io_burn, io_size*std::mega::num, usleep * std::kilo::num));
  for (auto& th: pool)
    th.join();

  return 0;
}
