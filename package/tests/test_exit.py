#! /usr/bin/env python3
#
# Copyright (C) 2018-2020 CERN
# License Apache2 - see LICENCE file

"""prmon test case for monitored process exit code"""

import argparse
import subprocess
import sys
import unittest

THE_TEST = None


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
                "Wrong return code from prmon (expected {0}".format(exit_code),
            )

    return ConfigurableProcessMonitor


def main():
    """Parse arguments and call test class generator"""
    parser = argparse.ArgumentParser(description="Configurable test runner")
    parser.add_argument("--exit-code", type=int, default=0)

    args = parser.parse_args()
    # Stop unittest from being confused by the arguments
    sys.argv = sys.argv[:1]

    global THE_TEST
    THE_TEST = setup_configurable_test(args.exit_code)

    unittest.main()


if __name__ == "__main__":
    main()
