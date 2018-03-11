// Simple CPU burner to simulate workload
// for testing prmon

#include <cmath>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ratio>
#include <thread>
#include <vector>
#include <unistd.h>

#include <boost/program_options.hpp>

#include "burner.h"

namespace prog_opt = boost::program_options;

const float default_runtime = 10.0;

double burn(unsigned long iterations = 10'000'000lu) {
  // Perform a time wasting bit of maths
  // Use volatile to prevent the compiler from optimising away
  volatile double sum{0.0};
  double f;
  for (auto i = 0lu; i < iterations; ++i) {
    f = (double)(i+1) / iterations * 1.414;
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

int main(int argn, char *argv[]) {
  float runtime{};
  unsigned int threads{}, procs{};

  prog_opt::options_description desc(
      "burner is a simple cpu burner program that can run in multiple threads\n"
      "and/or processes.\n\n"
      "If both threads and procs are sepecified then each process runs\n"
      "multiple threads (so the load is threads * procs).\n\n"
      "Allowed options:");
  desc.add_options()
      ("help,h", "print this help message")
      ("threads,t", prog_opt::value<unsigned int>(&threads)->default_value(1),
          "run in N threads (setting 0 will used the hardware concurrency value)")
      ("procs,p", prog_opt::value<unsigned int>(&procs)->default_value(1),
          "run in N processes (setting 0 will used the hardware concurrency value)")
      ("time,r", prog_opt::value<float>(&runtime)->default_value(default_runtime), "run for T seconds")
  ;

  // Parse command line
  prog_opt::variables_map var_map;
  prog_opt::store(prog_opt::parse_command_line(argn, argv, desc), var_map);
  prog_opt::notify(var_map);

  if (var_map.count("help")) {
      std::cout << desc << "\n";
      return 1;
  }

  if (runtime < 0.0f) {
    std::cerr << "Program rum time cannot be negative" << std::endl;
    return 1;
  }

  // If threads or procs is set to zero, then use the hardware
  // concurrency value
  auto hw = std::thread::hardware_concurrency();
  if (threads==0) threads=hw;
  if (procs==0) procs=hw;

  std::cout << "Will run for " << runtime << "s using " << procs <<
      " process(es) and " << threads << " thread(s)" << std::endl;

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
    pool.push_back(std::thread(burn_for, runtime*std::kilo::num));
  for (auto& th: pool)
    th.join();

  return 0;
}
