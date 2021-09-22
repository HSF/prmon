#! /usr/bin/env python3
#
# Copyright (C) 2018-2020 CERN
# License Apache2 - see LICENCE file

"""prmon unitest for memory consumption"""

import argparse
import json
import subprocess
import sys
import unittest


def setup_configurable_test(
    procs=4, malloc_mb=100, write_fraction=0.5, sleep=10, slack=0.9, interval=1
):
    """Wrap the class definition in a function to allow arguments to be passed"""

    class ConfigurableProcessMonitor(unittest.TestCase):
        """Test class for a specific set of parameters"""

        def check_memory_limits(self, name, value, expected, slack):
            """Check values against expectation with a tolerance"""
            max_value = expected * (1.0 + (1.0 - slack))
            min_value = expected * slack
            self.assertLess(
                value,
                max_value,
                f"Too high a value for {name} "
                f"(expected maximum of {max_value}, got {value})",
            )
            self.assertGreater(
                value,
                min_value,
                f"Too low a value for {name} "
                f"(expected maximum of {min_value}, got {value})",
            )

        def test_run_test_with_params(self):
            """Actual test runner"""
            burn_cmd = [
                "./mem-burner",
                "--sleep",
                str(sleep),
                "--malloc",
                str(malloc_mb),
                "--writef",
                str(write_fraction),
            ]
            if procs != 1:
                burn_cmd.extend(["--procs", str(procs)])

            prmon_cmd = ["../prmon", "--interval", str(interval), "--"]
            prmon_cmd.extend(burn_cmd)
            prmon_p = subprocess.Popen(prmon_cmd, shell=False)

            prmon_rc = prmon_p.wait()

            self.assertEqual(prmon_rc, 0, "Non-zero return code from prmon")

            with open("prmon.json") as infile:
                prmon_json = json.load(infile)

                # Memory tests
                vmwm_expect = (
                    malloc_mb * procs * 1024 + 10000 * procs
                )  # Small uplift for program itself
                self.check_memory_limits(
                    "vmem", prmon_json["Max"]["vmem"], vmwm_expect, slack
                )

                rss_expect = malloc_mb * procs * 1024 * write_fraction
                self.check_memory_limits(
                    "rss", prmon_json["Max"]["rss"], rss_expect, slack
                )

                pss_expect = malloc_mb * 1024 * write_fraction
                self.check_memory_limits(
                    "pss", prmon_json["Max"]["pss"], pss_expect, slack
                )

    return ConfigurableProcessMonitor


def main_parse_args_and_get_test():
    """Parse arguments and call test class generator
    returning the test case (which is unusual for a
    main() function)"""
    parser = argparse.ArgumentParser(description="Configurable memory test runner")
    parser.add_argument("--procs", type=int, default=4)
    parser.add_argument("--malloc", type=int, default=100)
    parser.add_argument("--writef", type=float, default=0.5)
    parser.add_argument("--sleep", type=int, default=10)
    parser.add_argument("--slack", type=float, default=0.85)
    parser.add_argument("--interval", type=int, default=1)

    args = parser.parse_args()
    # Stop unittest from being confused by the arguments
    sys.argv = sys.argv[:1]

    return setup_configurable_test(
        args.procs, args.malloc, args.writef, args.sleep, args.slack, args.interval
    )


if __name__ == "__main__":
    # As unitest will only run tests in the global namespace
    # we return the test instance from main()
    the_test = main_parse_args_and_get_test()
    unittest.main()
