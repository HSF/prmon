#! /usr/bin/env python3
"""Generate pre-cooked pseudo /proc output for use with tests"""

import os
import time
import random
import gc
import shutil
import argparse

pid = 1729


def make_stat(pid, proc_pid, fixed_value, rand=False):
    stat_fname = os.path.join(proc_pid, "stat")
    # if rand:
    with open(stat_fname, "w") as f:
        # Writing some pid and process name although never used
        print(pid, file=f, end=" ")  # 0 pid
        print("(python3)", file=f, end=" ")  # 1 Executable filename
        # 2 to 12 are never used (as are 0 and 1) -> set to 0
        print("0 " * 11, file=f, end="")

        # Define upper limit for random generation of integers for utime and stime
        time_lim = 10000000
        # 13 utime - Time in user mode
        # 14 stime - Time in kernel mode
        # 15 cutime - Time of waited-for children in user mode
        # 16 cstime - Time of waited-for children in kernel mode
        print(
            " ".join(
                [
                    str(random.randint(1, time_lim)) if rand else str(fixed_value)
                    for i in range(4)
                ]
            ),
            file=f,
            end=" ",
        )

        # 17 and 18 are never used -> set to 0
        print("0 0", file=f, end=" ")

        # 19 num_threads - Number of threads of the process
        num_threads = 1
        print(num_threads, file=f, end=" ")

        # 20 never used -> set to 0
        print("0", file=f, end=" ")

        # 21 starttime (uptime in utils.h) (last field that is read by any monitor)
        print(random.randint(1, time_lim) if rand else fixed_value, file=f)


def make_io(proc_pid, fixed_value, rand=False):
    io_fname = os.path.join(proc_pid, "io")
    fields = [
        "rchar",
        "wchar",
        "syscr",
        "syscw",
        "read_bytes",
        "write_bytes",
        "cancelled_write_bytes",
    ]
    char_lim = 1000
    with open(io_fname, "w") as f:
        for field in fields:
            print(
                f"{field}: {random.randint(0, char_lim) if rand else fixed_value}",
                file=f,
            )


def make_smaps(proc_pid, fixed_value, rand=False):
    smaps_fname = os.path.join(proc_pid, "smaps")
    fields = ["Size", "Pss", "Rss", "Swap"]
    mem_lim = 1000
    with open(smaps_fname, "w") as f:
        for field in fields:
            print(
                f"{field}: {random.randint(0, mem_lim) if rand else fixed_value} kB",
                file=f,
            )


def make_net(proc_net, fixed_value, rand=False):
    net_params = ["rx_bytes", "tx_bytes", "rx_packets", "tx_packets"]
    net_lim = 1000000
    for param in net_params:
        net_fname = os.path.join(proc_net, param)
        with open(net_fname, "w") as f:
            print(random.randint(0, net_lim) if rand else fixed_value, file=f)


def make_nvidia(proc_nvidia, fixed_value_sm_mem, fixed_value_fb,
                rand_sm_mem=False, rand_fb=False):
    # idx
    print(proc_nvidia, fixed_value_sm_mem, fixed_value_fb, rand_sm_mem, rand_fb)
    smi_fname = os.path.join(proc_nvidia, "smi")
    pct_lim = 100
    memory_lim = 10000
    with open(smi_fname, "w") as f:
        params = [
            0,  # idx
            pid,  # pid
            "G",  # type
            random.randint(0, pct_lim) if rand_sm_mem else fixed_value_sm_mem,  # sm
            random.randint(0, pct_lim) if rand_sm_mem else fixed_value_sm_mem,  # mem
            # The following are not monitored metrics
            "-",  # enc
            "-",  # dec
            "-",  # jpg
            "-",  # ofa
            random.randint(0, memory_lim) if rand_fb else fixed_value_fb,  # fb
            0,  # ccpm
            "python3",  # command
        ]
        for param in params:
            print(param, file=f, end=" ")
        print("", file=f)

def make_machstat(proc_mach, fixed_value, rand=False):
    mach_fname = os.path.join(proc_mach, "stat")
    lim = 2**64 - 1
    if rand:
        user, nice, system = random.randint(1,lim)
    else:
        user, nice, system = fixed_value, fixed_value, fixed_value
    with open(mach_fname, "w") as f:
        print("cpu", file = f, end = " ") # Relevant line
        # 1 utime - Time in user mode
        # 2 ntime - Time in nice mode
        # 3 stime - Time in system mode
        print(f"{user} {nice} {system}", file = f, end = " ")

        # 4 - 10 are never used, set to 0
        print("0 " * 7, file = f, end = "")

def make_power(power_dir, fixed_value, rand = False):
    power_fname = os.path.join(power_dir, "ipmi.prom")
    if rand:
        dcmi, ipmi, hwmon = (random.randint(0, 1000) for _ in range(3))
        rapl_0, rapl_1 = (random.randint(0, 1000000) for _ in range(3))
    else:
        dcmi, ipmi, hwmon, rapl_0, rapl_1 = fixed_value, fixed_value, fixed_value, fixed_value, fixed_value
    with open(power_fname, "w") as f:
        print("# HELP node_hwmon_power_watt Hardware monitor for power consumption", file=f)
        print("# TYPE node_hwmon_power_watt gauge", file=f)
        print(f'node_hwmon_power_watt{{chip="power_supply_bat1",sensor="power1"}} {hwmon}', file=f)
        print("# HELP node_rapl_package_joules_total Current RAPL package value in joules", file=f)
        print("# TYPE node_rapl_package_joules_total counter", file=f)
        print(f'node_rapl_package_joules_total{{index="0",path="/sys/class/powercap/intel-rapl:0"}} {rapl_0}', file=f)
        print(f'node_rapl_package_joules_total{{index="1",path="/sys/class/powercap/intel-rapl:1"}} {rapl_1}', file=f)
        print("# HELP ipmi_dcmi_power_consumption_watts Current power consumption in watts", file=f)
        print("# TYPE ipmi_dcmi_power_consumption_watts gauge", file=f)
        print(f"ipmi_dcmi_power_consumption_watts {dcmi}", file=f)
        print("# HELP ipmi_power_watts Power reading of the sensor in watts", file=f)
        print("# TYPE ipmi_power_watts gauge", file=f)
        print(f'ipmi_power_watts{{id="1",name="Pwr Consumption"}} {ipmi}', file=f)

def createPath(new_dir):
    dir_name = os.path.join(os.getcwd(), "precooked_tests")
    if not os.path.isdir(dir_name):
        os.mkdir(dir_name)
    dir_name = os.path.join(dir_name, new_dir)
    if os.path.isdir(dir_name):
        shutil.rmtree(dir_name)
    os.mkdir(dir_name)
    return dir_name


def createTestMonotonic():
    # 3 iterations having first increasing and then decreasing stats
    dir_name = createPath("drop")

    interval = 0
    i = 1
    iterationCount = 3
    while i <= iterationCount:
        # New directory for each iteration
        new_dir = os.path.join(dir_name, str(i))
        os.mkdir(new_dir)
        # Create /proc equivalent directory
        proc = os.path.join(new_dir, "proc")
        os.mkdir(proc)

        # Create /proc/pid equivalent directory
        proc_pid = os.path.join(proc, str(pid))
        net_dir = os.path.join(new_dir, "net")
        nvidia_dir = os.path.join(new_dir, "nvidia")
        power_dir = os.path.join(new_dir, "power")
        os.mkdir(proc_pid)
        os.mkdir(net_dir)
        os.mkdir(nvidia_dir)
        os.mkdir(power_dir)
        if i == 1:
            make_stat(pid, proc_pid, 5000000)
            make_io(proc_pid, 500)
            make_smaps(proc_pid, 5000)
            make_net(net_dir, 500000)
            make_nvidia(nvidia_dir, 50, 50)
            make_machstat(proc, 50000)
            make_power(power_dir, 500)
        elif i == 2:
            make_stat(pid, proc_pid, 10000000)
            make_io(proc_pid, 1000)
            make_smaps(proc_pid, 10000)
            make_net(net_dir, 1000000)
            make_nvidia(nvidia_dir, 100, 100)
            make_machstat(proc, 100000)
            make_power(power_dir, 1000)
        else:
            make_stat(pid, proc_pid, 2000000)
            make_io(proc_pid, 200)
            make_smaps(proc_pid, 2000)
            make_net(net_dir, 200000)
            make_nvidia(nvidia_dir, "-", 20)
            make_machstat(proc, 20000)
            make_power(power_dir, 200)
        gc.collect()
        time.sleep(interval)
        i += 1


def createTestRand(dir_name, iter):
    # 3 iterations having first increasing and then decreasing stats
    dir_name = createPath(dir_name)

    interval = 0
    i = 1
    for i in range(1, iter + 1):
        # New directory for each iteration
        new_dir = os.path.join(dir_name, str(i))
        os.mkdir(new_dir)
        # Create /proc equivalent directory
        proc = os.path.join(new_dir, "proc")
        os.mkdir(proc)

        # Create /proc/pid equivalent directory
        proc_pid = os.path.join(proc, str(pid))
        net_dir = os.path.join(new_dir, "net")
        nvidia_dir = os.path.join(new_dir, "nvidia")
        os.mkdir(proc_pid)
        os.mkdir(net_dir)
        os.mkdir(nvidia_dir)
        os.mkdir(mach_dir)
        os.mkdir(power_dir)
        make_stat(pid, proc_pid, 0, True)
        make_io(proc_pid, 0, True)
        make_smaps(proc_pid, 0, True)
        make_net(net_dir, 0, True)
        make_nvidia(nvidia_dir, 0, True)
        make_mach(mach_dir, 0, True)
        make_power(power_dir, 0, True)
        gc.collect()
        time.sleep(interval)


def main():
    parser = argparse.ArgumentParser(description="Precooked test generator")

    parser.add_argument("--dir", type=str, help="Name of the directory to be generated")

    parser.add_argument("--iter", type=int, help="Number of iterations to be generated")

    args = parser.parse_args()

    if args.dir is None and args.iter is not None:
        parser.error("The number of iterations has to be specified")
    if args.dir is not None and args.iter is None:
        parser.error("The name of the directory has to be specified")
    if args.dir is None:
        createTestMonotonic()
    else:
        createTestRand(args.dir, args.iter)


if __name__ == "__main__":
    main()
