// Simple memory 'burner' to simulate workload
// for testing prmon. Will allocate a large
// memory block, and write only to a certain
// fraction of it (test RSS vs VMEM). Children
// can be spawned that share pages with the
// parent (test PSS vs RSS).

#include <getopt.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <ratio>
#include <thread>

void SignalChildHandler(int /*signal*/) {
  pid_t pid{1};
  while (pid > 0)
    pid = waitpid((pid_t)-1, NULL, WNOHANG);
}

int main(int argc, char* argv[]) {
  // Default values
  const unsigned int default_malloc_mb = 10;
  const float default_mem_write_fraction = 0.5;
  const unsigned int default_sleep_s = 10;
  const unsigned int default_procs = 1;

  unsigned int malloc_mb{default_malloc_mb};
  float mem_write_fraction{default_mem_write_fraction};
  float sleep_s{default_sleep_s};
  unsigned int procs{default_procs};
  int do_help{0};

  static struct option long_options[] = {
      {"malloc", required_argument, NULL, 'm'},
      {"writef", required_argument, NULL, 'f'},
      {"sleep", required_argument, NULL, 's'},
      {"procs", required_argument, NULL, 'p'},
      {"help", no_argument, NULL, 'h'},
      {0, 0, 0, 0}};

  char c;
  while ((c = getopt_long(argc, argv, "m:f:s:p:h", long_options, NULL)) != -1) {
    switch (c) {
      case 'm':
        if (std::stol(optarg) < 0) {
          std::cerr << "malloc (MB) must be greater than 0 (--help for usage)"
                    << std::endl;
          return 1;
        }
        malloc_mb = std::stol(optarg);
        break;
      case 'f':
        mem_write_fraction = std::stof(optarg);
        if (mem_write_fraction < 0.0 || mem_write_fraction > 1.0) {
          std::cerr
              << "memory write fraction parameter must be between 0 and 1 "
                 "(--help for usage)"
              << std::endl;
          return 1;
        }
        break;
      case 's':
        if (std::stoi(optarg) < 0) {
          std::cerr << "sleep parameter must be greater than or equal to 0 "
                       "(--help for usage)"
                    << std::endl;
          return 1;
        }
        sleep_s = std::stoi(optarg);
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
        << "mem-burner is a simple memory grabber program that is used to \n"
        << "test prmon\n"
        << std::endl;
    std::cout
        << "Options:\n"
        << " [--malloc, -m N]    Number of megabytes of memory to allocate "
           "(default "
        << default_malloc_mb << "MB)\n"
        << " [--writef, -f N]    Fraction of memory to actually write to "
           "(default "
        << default_mem_write_fraction << ")\n"
        << " [--procs, -p N]     Number of processes to run (default "
        << default_procs << ")\n"
        << " [--sleep, -s N]    Sleep (in seconds) before terminating "
        << default_sleep_s << ")\n"
        << "If procs is set to 0, the hardware concurrency "
           "value is used."
        << std::endl;
    return 0;
  }

  // If threads or procs is set to zero, then use the hardware
  // concurrency value
  auto hw = std::thread::hardware_concurrency();
  if (procs == 0) procs = hw;

  std::cout << "Will allocate " << malloc_mb << "MB of memory and write to "
            << mem_write_fraction << " of it ("
            << malloc_mb * mem_write_fraction << "MB); " << procs
            << " processes will be used." << std::endl;

  // Allocate memory block and write to only some of it
  unsigned long malloc_bytes = malloc_mb * std::mega::num;
  unsigned long write_bytes = malloc_bytes * mem_write_fraction;

  uint8_t* mem_block;
  if (!(mem_block = (uint8_t*)std::malloc(malloc_bytes))) {
    std::cerr << "Malloc of " << malloc_bytes << " failed." << std::endl;
    return 1;
  }

  for (unsigned long i = 0; i < write_bytes; ++i) mem_block[i] = 0xaa;

  // Now fork child processes, these will
  // do nothing special, so they share the malloc
  // block with their parent
  pid_t pid = getpid();
  unsigned int children{0};
  if (procs > 1) {
    while (children < procs - 1 && pid != 0) {
      pid = fork();
      ++children;
    }
  }

  // Parent should respond to child exits
  if (pid) signal(SIGCHLD, SignalChildHandler);

  // Now sleep and exit
  sleep(sleep_s);
  std::free(mem_block);

  return 0;
}
