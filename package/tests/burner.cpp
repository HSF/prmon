// Simple CPU burner to simulate workload
// for testing prmon

#include <getopt.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <ratio>
#include <string>
#include <thread>
#include <vector>

#include "burner.h"

double burn(unsigned long iterations = 10000000lu) {
  // Perform a time wasting bit of maths
  // Use volatile to prevent the compiler from optimising away
  volatile double sum{0.0};
  double f;
  for (auto i = 0lu; i < iterations; ++i) {
    f = (double)(i + 1) / iterations * 1.414;
    sum += std::sin(std::log(f));
  }
  return sum;
}

double burn_for(float ms_interval = 1.0) {
  // Use volatile to prevent the compiler from optimising away
  volatile double burn_result{0.0};
  std::chrono::duration<float, std::milli> chrono_interval(ms_interval);
  auto start = std::chrono::system_clock::now();
  while (std::chrono::system_clock::now() - start < chrono_interval)
    burn_result += burn(5000);

  return burn_result;
}

void SignalChildHandler(int /*signal*/) {
  pid_t pid{1};
  while (pid>0)
    pid = waitpid((pid_t)-1, NULL, WNOHANG);
}

int main(int argc, char* argv[]) {
  // Default values
  const float default_runtime = 10.0f;
  const float default_child_runtime_fraction = 1.0;
  const unsigned int default_threads = 1;
  const unsigned int default_procs = 1;

  float runtime{default_runtime}, child_runtime_fraction{default_child_runtime_fraction};
  unsigned int threads{default_threads}, procs{default_procs};
  int do_help{0};

  static struct option long_options[] = {
      {"threads", required_argument, NULL, 't'},
      {"procs", required_argument, NULL, 'p'},
      {"time", required_argument, NULL, 'r'},
      {"child-fraction", required_argument, NULL, 'c'},
      {"help", no_argument, NULL, 'h'},
      {0, 0, 0, 0}};

  char c;
  while ((c = getopt_long(argc, argv, "t:p:c:r:h", long_options, NULL)) != -1) {
    switch (c) {
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
      case 'c':
        child_runtime_fraction = std::stof(optarg);
        if (child_runtime_fraction <= 0 || child_runtime_fraction > 1.0) {
          std::cerr
              << "child runtime fraction must be in range (0,1.0] (--help for usage)"
              << std::endl;
          return 1;
        }
        break;
      case 'r':
        runtime = std::stof(optarg);
        if (runtime <= 0) {
          std::cerr
              << "runtime parameter must be greater than 0 (--help for usage)"
              << std::endl;
          return 1;
        }
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
        << "burner is a simple cpu burner program that can run in multiple "
           "threads\n"
           "and/or processes.\n\n"
           "If both threads and procs are specified then each process runs\n"
           "multiple threads (so the load is threads * procs).\n"
        << std::endl;
    std::cout << "Options:\n"
              << " [--threads, -t N]  Number of threads to run (default "
              << default_threads << ")\n"
              << " [--procs, -p N]    Number of processes to run (default "
              << default_procs << ")\n"
              << " [--child-fraction, -c F]     Run each child for a fraction F of the parent runtime (default "
              << default_child_runtime_fraction << ")\n"
              << " [--time, -r T]     Run for T seconds (default "
              << default_runtime << ")\n\n"
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

  std::cout << "Will run for " << runtime << "s using " << procs
            << " process(es) and " << threads << " thread(s)" << std::endl;
  if (child_runtime_fraction < 1.0 && procs > 1)
    std::cout << "Children will run for " << runtime * child_runtime_fraction
              << "s" << std::endl;

  // First fork child processes
  pid_t pid = getpid();
  if (procs > 1) {
    unsigned int children{0};
    while (children < procs - 1 && pid != 0) {
      pid = fork();
      ++children;
    }
  }
  if (pid)
    signal(SIGCHLD, SignalChildHandler);

  // Each process runs the requested number of threads
  std::vector<std::thread> pool;
  for (unsigned int i = 0; i < threads; ++i)
    pool.push_back(std::thread(burn_for, runtime * std::kilo::num * (pid ? 1.0 : child_runtime_fraction)));

  for (auto& th : pool) th.join();

  return 0;
}
