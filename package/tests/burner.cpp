// Simple CPU burner to simulate workload
// for testing prmon

#include <cmath>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ratio>

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
  int threads{}, procs{};

  prog_opt::options_description desc("Allowed options");
  desc.add_options()
      ("help", "print this help message")
      ("threads", prog_opt::value<int>(&threads)->default_value(1), "run in N threads")
      ("procs", prog_opt::value<int>(&procs)->default_value(1), "run in N processes")
      ("time", prog_opt::value<float>(&runtime)->default_value(default_runtime), "run for T seconds")
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

  std::cout << "Will run for " << runtime << "s using " << procs <<
      " process(es) and " << threads << " thread(s)" << std::endl;

  burn_for(runtime * std::kilo::num);

  return 0;
}
