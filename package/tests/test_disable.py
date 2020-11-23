#! /usr/bin/env python3
#
# Copyright (C) 2020 CERN
# License Apache2 - see LICENCE file

"""prmon test harness to check monitors can be disabled correctly"""

import argparse
import json
import subprocess
import sys
import unittest


def setup_configurable_test(disable=[]):
    """Wrap the class definition in a function to allow arguments to be passed"""

    class ConfigurableProcessMonitor(unittest.TestCase):
        """Test class for a specific set of parameters"""

        def test_run_test_with_params(self):
            """Actual test runner"""
            burn_cmd = ["./burner", "--time", "3"]

            prmon_cmd = ["../prmon", "--interval", "1"]
            for d in disable:
                prmon_cmd.extend(("-d", d))
            prmon_cmd.append("--")
            prmon_cmd.extend(burn_cmd)
            prmon_p = subprocess.Popen(prmon_cmd, shell=False)

            prmon_rc = prmon_p.wait()

            self.assertEqual(prmon_rc, 0, "Non-zero return code from prmon")

            with open("prmon.json") as infile:
                prmon_json = json.load(infile)

                # Check we don't have fields corresponding to disabled monitors
                monitor_fields = {
                    "countmon": "nprocs",
                    "cpumon": "stime",
                    "iomon": "rchar",
                    "memmon": "vmem",
                    "netmon": "rx_bytes",
                    "nvidiamon": "ngpus",
                }
                for d in disable:
                    if d in monitor_fields:
                        self.assertFalse(monitor_fields[d] in prmon_json["Max"])

    return ConfigurableProcessMonitor


def main_parse_args_and_get_test():
    """Parse arguments and call test class generator
    returning the test case (which is unusual for a
    main() function)"""
    parser = argparse.ArgumentParser(description="Configurable test runner")
    parser.add_argument("--disable", nargs="+")

    args = parser.parse_args()
    # Stop unittest from being confused by the arguments
    sys.argv = sys.argv[:1]

    return setup_configurable_test(args.disable)


if __name__ == "__main__":
    # As unitest will only run tests in the global namespace
    # we return the test instance from main()
    the_test = main_parse_args_and_get_test()
    unittest.main()
