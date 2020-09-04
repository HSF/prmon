#! /usr/bin/env python3
#
# Copyright (C) 2018-2020 CERN
# License Apache2 - see LICENCE file

"""prmon test harness to count number of processes"""

import argparse
import json
import subprocess
import sys
import unittest

THE_TEST = None


def setup_configurable_test(threads=1, procs=1, time=10.0, interval=1, invoke=False):
    """Wrap the class definition in a function to allow arguments to be passed"""

    class ConfigurableProcessMonitor(unittest.TestCase):
        """Test class for a specific set of parameters"""

        def test_run_test_with_params(self):
            """Actual test runner"""
            burn_cmd = ["./burner", "--time", str(time)]
            if threads != 1:
                burn_cmd.extend(["--threads", str(threads)])
            if procs != 1:
                burn_cmd.extend(["--procs", str(procs)])

            prmon_cmd = ["../prmon", "--interval", str(interval)]
            if invoke:
                prmon_cmd.append("--")
                prmon_cmd.extend(burn_cmd)
                prmon_p = subprocess.Popen(prmon_cmd, shell=False)

                prmon_rc = prmon_p.wait()
            else:
                burn_p = subprocess.Popen(burn_cmd, shell=False)

                prmon_cmd.extend(["--pid", str(burn_p.pid)])
                prmon_p = subprocess.Popen(prmon_cmd, shell=False)

                _ = burn_p.wait()
                prmon_rc = prmon_p.wait()

            self.assertEqual(prmon_rc, 0, "Non-zero return code from prmon")

            with open("prmon.json") as infile:
                prmon_json = json.load(infile)

                # Simple Process and Thread counting test
                total_proc = prmon_json["Max"]["nprocs"]
                total_thread = prmon_json["Max"]["nthreads"]
                expect_proc = procs
                expect_thread = procs * threads
                self.assertAlmostEqual(
                    total_proc,
                    expect_proc,
                    msg="Inconsistent value for number of processes "
                    "(expected {0}, got {1})".format(expect_proc, total_proc),
                )
                self.assertAlmostEqual(
                    total_thread,
                    expect_thread,
                    msg="Inconsistent value for number of total threads "
                    "(expected {0}, got {1}".format(expect_thread, total_thread),
                )

    return ConfigurableProcessMonitor


def main():
    """Parse arguments and call test class generator"""
    parser = argparse.ArgumentParser(description="Configurable test runner")
    parser.add_argument("--threads", type=int, default=1)
    parser.add_argument("--procs", type=int, default=1)
    parser.add_argument("--time", type=float, default=10)
    parser.add_argument("--interval", type=int, default=1)
    parser.add_argument("--invoke", dest="invoke", action="store_true", default=False)

    args = parser.parse_args()
    # Stop unittest from being confused by the arguments
    sys.argv = sys.argv[:1]

    # unittest will only run tests that live in the global namespace
    global THE_TEST
    THE_TEST = setup_configurable_test(
        args.threads, args.procs, args.time, args.interval, args.invoke
    )

    unittest.main()


if __name__ == "__main__":
    main()
