#include <sys/types.h>
#include <unistd.h>

#include <vector>
#include <utility>
#include <unordered_map>

using monitor_switch_t = std::unordered_map<std::string, bool>;

bool kernel_proc_pid_test(const pid_t pid);
std::vector<pid_t> pstree_pids(const pid_t mother_pid);
std::vector<pid_t> offspring_pids(const pid_t mother_pid);

const monitor_switch_t parse_monitor_switches(std::vector<std::string>, bool);
const std::pair<std::string, bool> monitor_switch_state(const std::string monitor);
