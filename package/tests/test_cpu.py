#! /usr/bin/env python3
#
# Copyright (C) 2018-2020 CERN
# License Apache2 - see LICENCE file

"""Test harness for process and thread burning tests"""

import argparse
import json
import subprocess
import sys
import unittest

THE_TEST = None


def setup_configurable_test(
    threads=1,
    procs=1,
    child_fraction=1.0,
    time=10.0,
    slack=0.75,
    interval=1,
    invoke=False,
    units=False,
):
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
            if child_fraction != 1.0:
                burn_cmd.extend(["--child-fraction", str(child_fraction)])

            prmon_cmd = ["../prmon", "--interval", str(interval)]
            if units:
                prmon_cmd.append("--units")
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

                # CPU time tests
                total_cpu = prmon_json["Max"]["utime"] + prmon_json["Max"]["stime"]
                expect_cpu = (1.0 + (procs - 1) * child_fraction) * time * threads
                self.assertLessEqual(
                    total_cpu,
                    expect_cpu,
                    "Too high value for CPU time "
                    "(expected maximum of {0}, got {1})".format(expect_cpu, total_cpu),
                )
                self.assertGreaterEqual(
                    total_cpu,
                    expect_cpu * slack,
                    "Too low value for CPU time "
                    "(expected minimum of {0}, got {1}".format(
                        expect_cpu * slack, total_cpu
                    ),
                )
                # Wall time tests
                total_wall = prmon_json["Max"]["wtime"]
                self.assertLessEqual(
                    total_wall,
                    time,
                    "Too high value for wall time "
                    "(expected maximum of {0}, got {1})".format(time, total_wall),
                )
                self.assertGreaterEqual(
                    total_wall,
                    time * slack,
                    "Too low value for wall time "
                    "(expected minimum of {0}, got {1}".format(
                        time * slack, total_wall
                    ),
                )

                # Unit test
                if units:
                    for group in ("Max", "Avg"):
                        value_params = set(prmon_json[group].keys())
                        unit_params = set(prmon_json["Units"][group].keys())
                        missing = value_params - unit_params
                        self.assertEqual(
                            len(missing),
                            0,
                            "Wrong number of unit values for '{0}'".format(group)
                            + " - missing parameters are {0}".format(missing),
                        )
                        extras = unit_params - value_params
                        self.assertEqual(
                            len(extras),
                            0,
                            "Wrong number of unit values for '{0}'".format(group)
                            + " - extra parameters are {0}".format(extras),
                        )

    return ConfigurableProcessMonitor


def main():
    """Parse arguments and call test class generator"""
    parser = argparse.ArgumentParser(description="Configurable test runner")
    parser.add_argument("--threads", type=int, default=1)
    parser.add_argument("--procs", type=int, default=1)
    parser.add_argument("--child-fraction", type=float, default=1.0)
    parser.add_argument("--time", type=float, default=10)
    parser.add_argument("--slack", type=float, default=0.7)
    parser.add_argument("--interval", type=int, default=1)
    parser.add_argument("--invoke", action="store_true")
    parser.add_argument("--units", action="store_true")

    args = parser.parse_args()
    # Stop unittest from being confused by the arguments
    sys.argv = sys.argv[:1]

    # unittest will only run tests that live in the global namespace
    global THE_TEST
    THE_TEST = setup_configurable_test(
        args.threads,
        args.procs,
        args.child_fraction,
        args.time,
        args.slack,
        args.interval,
        args.invoke,
        args.units,
    )

    unittest.main()


if __name__ == "__main__":
    main()
