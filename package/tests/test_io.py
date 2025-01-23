#! /usr/bin/env python3
#
# Copyright (C) 2018-2025 CERN
# License Apache2 - see LICENCE file

"""prmon test for measuring i/o"""

import argparse
import json
import subprocess
import sys
import unittest


def setup_configurable_test(
    io=10, threads=1, procs=1, usleep=10, pause=1, slack=0.95, interval=1
):
    """Wrap the class definition in a function to allow arguments to be passed"""

    class ConfigurableProcessMonitor(unittest.TestCase):
        """Test class for a specific set of parameters"""

        def test_run_test_with_params(self):
            """Actual test runner"""
            burn_cmd = ["./io-burner", "--io", str(io)]
            burn_cmd.extend(["--threads", str(threads)])
            burn_cmd.extend(["--procs", str(procs)])
            burn_cmd.extend(["--usleep", str(usleep)])
            burn_cmd.extend(["--pause", str(pause)])
            burn_p = subprocess.Popen(burn_cmd, shell=False)

            prmon_cmd = [
                "../prmon",
                "--interval",
                str(interval),
                "--pid",
                str(burn_p.pid),
            ]
            prmon_p = subprocess.Popen(prmon_cmd, shell=False)

            burn_p.wait()
            prmon_rc = prmon_p.wait()

            self.assertEqual(prmon_rc, 0, "Non-zero return code from prmon")

            with open("prmon.json") as infile:
                prmon_json = json.load(infile)

                # IO tests
                expected_bytes = io * threads * procs * slack * 1.0e6
                self.assertGreaterEqual(
                    prmon_json["Max"]["wchar"],
                    expected_bytes,
                    "Too low value for IO bytes written "
                    f"(expected minimum of {expected_bytes}, "
                    f"got {prmon_json['Max']['wchar']})",
                )
                self.assertGreaterEqual(
                    prmon_json["Max"]["rchar"],
                    expected_bytes,
                    "Too low value for IO bytes read "
                    f"(expected minimum of {expected_bytes}, "
                    f"got {prmon_json['Max']['rchar']})",
                )

    return ConfigurableProcessMonitor


def main_parse_args_and_get_test():
    """Parse arguments and call test class generator
    returning the test case (which is unusual for a
    main() function)"""
    parser = argparse.ArgumentParser(description="Configurable test runner")
    parser.add_argument("--threads", type=int, default=1)
    parser.add_argument("--procs", type=int, default=1)
    parser.add_argument("--io", type=int, default=10)
    parser.add_argument("--usleep", type=int, default=10)
    parser.add_argument("--pause", type=float, default=1)
    parser.add_argument("--slack", type=float, default=0.9)
    parser.add_argument("--interval", type=int, default=1)
    args = parser.parse_args()
    # Stop unittest from being confused by the arguments
    sys.argv = sys.argv[:1]

    return setup_configurable_test(
        args.io,
        args.threads,
        args.procs,
        args.usleep,
        args.pause,
        args.slack,
        args.interval,
    )


if __name__ == "__main__":
    # As unitest will only run tests in the global namespace
    # we return the test instance from main()
    the_test = main_parse_args_and_get_test()
    unittest.main()
