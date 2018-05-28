#include <sys/types.h>
#include <unistd.h>

#include <vector>

bool kernel_proc_pid_test(const pid_t pid);
std::vector<pid_t> pstree_pids(const pid_t mother_pid);
std::vector<pid_t> offspring_pids(const pid_t mother_pid);
