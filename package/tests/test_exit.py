#! /usr/bin/env python3
#
# Copyright (C) 2018-2025 CERN
# License Apache2 - see LICENCE file

"""prmon test case for monitored process exit code"""

import argparse
import subprocess
import sys
import unittest


def setup_configurable_test(exit_code=0):
    """Wrap the class definition in a function to allow arguments to be passed"""

    class ConfigurableProcessMonitor(unittest.TestCase):
        """Wrap the class definition in a function to allow arguments to be passed"""

        def test_run_test_with_params(self):
            """Actual test runner"""
            child_cmd = ["sh", "-c", "sleep 3 && exit {0}".format(exit_code)]

            prmon_cmd = ["../prmon", "--interval", "1"]
            prmon_cmd.append("--")
            prmon_cmd.extend(child_cmd)
            prmon_p = subprocess.Popen(prmon_cmd, shell=False)
            prmon_rc = prmon_p.wait()

            self.assertEqual(
                prmon_rc,
                exit_code,
                f"Wrong return code from prmon (expected {exit_code}",
            )

    return ConfigurableProcessMonitor


def main_parse_args_and_get_test():
    """Parse arguments and call test class generator
    returning the test case (which is unusual for a
    main() function)"""
    parser = argparse.ArgumentParser(description="Configurable test runner")
    parser.add_argument("--exit-code", type=int, default=0)

    args = parser.parse_args()
    # Stop unittest from being confused by the arguments
    sys.argv = sys.argv[:1]

    return setup_configurable_test(args.exit_code)


if __name__ == "__main__":
    # As unitest will only run tests in the global namespace
    # we return the test instance from main()
    the_test = main_parse_args_and_get_test()
    unittest.main()
