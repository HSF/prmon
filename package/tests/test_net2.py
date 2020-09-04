#! /usr/bin/env python2
#
# Copyright (C) 2018-2020 CERN
# License Apache2 - see LICENCE file
#
# Test script for network IO (Python2 version)
from __future__ import print_function, unicode_literals

import argparse
import json
import os
import signal
import subprocess
import sys
import unittest

THE_TEST = None


def setup_configurable_test(
    blocks=None, requests=10, sleep=None, pause=None, slack=0.95, interval=1
):
    """Wrap the class definition in a function to allow arguments to be passed"""

    class ConfigurableProcessMonitor(unittest.TestCase):
        """Test class for a specific set of parameters"""

        def setUp(self):
            """Start the simple python HTTP CGI server"""
            http_server_cmd = ["python", "-m", "CGIhttp_server"]
            self.http_server = subprocess.Popen(http_server_cmd, shell=False)

        def tearDown(self):
            """Kill http server"""
            os.kill(self.http_server.pid, signal.SIGTERM)

        def test_run_test_with_params(self):
            """Test class for a specific set of parameters"""
            burn_cmd = ["python", "./net_burner2.py"]
            if requests:
                burn_cmd.extend(["--requests", str(requests)])
            if pause:
                burn_cmd.extend(["--pause", str(pause)])
            if sleep:
                burn_cmd.extend(["--sleep", str(sleep)])
            if blocks:
                burn_cmd.extend(["--blocks", str(blocks)])
            burn_p = subprocess.Popen(burn_cmd, shell=False)
            prmon_cmd = [
                "../prmon",
                "--interval",
                str(interval),
                "--pid",
                str(burn_p.pid),
            ]
            prmon_p = subprocess.Popen(prmon_cmd, shell=False)

            _ = burn_p.wait()
            prmon_rc = prmon_p.wait()

            self.assertEqual(prmon_rc, 0, "Non-zero return code from prmon")

            with open("prmon.json") as infile:
                prmon_json = json.load(infile)

                # Network tests
                expected_bytes = 1025000 * requests * slack
                self.assertGreaterEqual(
                    prmon_json["Max"]["rx_bytes"],
                    expected_bytes,
                    "Too low value for rx bytes "
                    "(expected minimum of {0}, got {1})".format(
                        expected_bytes, prmon_json["Max"]["rx_bytes"]
                    ),
                )
                self.assertGreaterEqual(
                    prmon_json["Max"]["tx_bytes"],
                    expected_bytes,
                    "Too low value for tx bytes "
                    "(expected minimum of {0}, got {1})".format(
                        expected_bytes, prmon_json["Max"]["tx_bytes"]
                    ),
                )

    return ConfigurableProcessMonitor


def main():
    """Parse arguments and call test class generator"""
    parser = argparse.ArgumentParser(
        description="Configurable test runner for network access"
    )
    parser.add_argument("--blocks", type=int)
    parser.add_argument("--requests", type=int, default=10)
    parser.add_argument("--sleep", type=float)
    parser.add_argument("--pause", type=float)
    parser.add_argument("--slack", type=float, default=0.90)
    parser.add_argument("--interval", type=int, default=1)
    args = parser.parse_args()
    # Stop unittest from being confused by the arguments
    sys.argv = sys.argv[:1]

    # unittest will only run tests that live in the global namespace
    global THE_TEST
    THE_TEST = setup_configurable_test(
        args.blocks, args.requests, args.sleep, args.pause, args.slack, args.interval
    )

    unittest.main()


if __name__ == "__main__":
    main()
