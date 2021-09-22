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


def make_nvidia(proc_nvidia, fixed_value, rand=False):
    # idx
    smi_fname = os.path.join(proc_nvidia, "smi")
    memory_lim = 10000
    with open(smi_fname, "w") as f:
        params = [
            0,  # idx
            pid,  # pid
            "G",  # type
            random.randint(0, memory_lim) if rand else fixed_value,  # sm
            random.randint(0, memory_lim) if rand else fixed_value,  # mem
            # enc, dec are not monitored metrics
            0,  # enc
            0,  # dec
            random.randint(0, memory_lim) if rand else fixed_value,  # fb
            "python3",  # command
        ]
        for param in params:
            print(param, file=f, end=" ")
        print("", file=f)


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
        os.mkdir(proc_pid)
        os.mkdir(net_dir)
        os.mkdir(nvidia_dir)
        if i == 1:
            make_stat(pid, proc_pid, 5000000)
            make_io(proc_pid, 500)
            make_smaps(proc_pid, 5000)
            make_net(net_dir, 500000)
            make_nvidia(nvidia_dir, 50)
        elif i == 2:
            make_stat(pid, proc_pid, 10000000)
            make_io(proc_pid, 1000)
            make_smaps(proc_pid, 10000)
            make_net(net_dir, 1000000)
            make_nvidia(nvidia_dir, 100)
        else:
            make_stat(pid, proc_pid, 2000000)
            make_io(proc_pid, 200)
            make_smaps(proc_pid, 2000)
            make_net(net_dir, 200000)
            make_nvidia(nvidia_dir, 20)
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
        make_stat(pid, proc_pid, 0, True)
        make_io(proc_pid, 0, True)
        make_smaps(proc_pid, 0, True)
        make_net(net_dir, 0, True)
        make_nvidia(nvidia_dir, 0, True)
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
