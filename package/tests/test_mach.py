#! /usr/bin/env python3
#
# Copyright (C) 2018-2026 CERN
# License Apache2 - see LICENCE file

"""Test harness for the machine level CPU time monitor (machmon)"""

import argparse
import json
import os
import subprocess
import sys
import unittest

from prmon_test_utils import dump_on_failure

MACH_KEYS = ("utime_total", "stime_total", "ntime_total")


def setup_configurable_test(
    threads=1,
    time=10.0,
    slack=0.7,
    interval=1,
    units=False,
):
    """Wrap the class definition in a function to allow arguments to be passed"""

    class MachineCPUMonitor(unittest.TestCase):
        """Test class for a specific set of parameters"""

        @dump_on_failure
        def test_run_test_with_params(self):
            """Actual test runner"""
            burn_cmd = ["./burner", "--time", str(time)]
            if threads != 1:
                burn_cmd.extend(["--threads", str(threads)])
            prmon_cmd = ["../prmon", "--interval", str(interval)]
            if units:
                prmon_cmd.append("--units")
            prmon_cmd.append("--")
            prmon_cmd.extend(burn_cmd)

            prmon_rc = subprocess.Popen(prmon_cmd, shell=False).wait()

            self.assertEqual(prmon_rc, 0, "Non-zero return code from prmon")

            with open("prmon.json") as infile:
                prmon_json = json.load(infile)

            maxes = prmon_json["Max"]
            for key in MACH_KEYS:
                self.assertIn(key, maxes, f"Missing '{key}' in the JSON summary")

            mach_cpu = sum(maxes[key] for key in MACH_KEYS)
            proc_cpu = maxes["utime"] + maxes["stime"]
            wall = maxes["wtime"]
            ncores = os.cpu_count() or 1

            # No more cores than the machine has
            expect_cpu = time * min(threads, ncores)

            # Counters are tuncated to seconds, add tolerance
            trunc = len(MACH_KEYS)

            # Allow for slack in CPU time
            self.assertGreaterEqual(
                mach_cpu + trunc,
                expect_cpu * slack,
                "Too low value for machine CPU time "
                f"(expected minimum of {expect_cpu * slack}, got {mach_cpu})",
            )
            self.assertGreaterEqual(
                mach_cpu + trunc,
                proc_cpu,
                "Machine CPU time is smaller than the monitored process CPU "
                f"time (machine {mach_cpu}, process {proc_cpu})",
            )
            # No more than every core busy during the whole run
            max_cpu = (wall + 1) * ncores
            self.assertLessEqual(
                mach_cpu,
                max_cpu,
                "Too high value for machine CPU time "
                f"(expected maximum of {max_cpu}, got {mach_cpu})",
            )

            # Monotonously increasing values in prmon.txt
            with open("prmon.txt") as infile:
                header = infile.readline().split()
                rows = [line.split() for line in infile if line.strip()]
            for key in MACH_KEYS:
                self.assertIn(key, header, f"Missing column '{key}' in prmon.txt")
                col = header.index(key)
                values = [float(row[col]) for row in rows]
                self.assertEqual(
                    values,
                    sorted(values),
                    f"Column '{key}' decreases in prmon.txt: {values}",
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
                        f"Wrong number of unit values for '{group}' - "
                        f"missing parameters are {missing}",
                    )
                    extras = unit_params - value_params
                    self.assertEqual(
                        len(extras),
                        0,
                        f"Wrong number of unit values for '{group}' - "
                        f"extra parameters are {extras}",
                    )

    return MachineCPUMonitor


def main_parse_args_and_get_test():
    """Parse arguments and call test class generator
    returning the test case (which is unusual for a
    main() function)"""
    parser = argparse.ArgumentParser(description="Configurable test runner")
    parser.add_argument("--threads", type=int, default=1)
    parser.add_argument("--time", type=float, default=10)
    parser.add_argument("--slack", type=float, default=0.7)
    parser.add_argument("--interval", type=int, default=1)
    parser.add_argument("--units", action="store_true")

    args = parser.parse_args()
    # Stop unittest from being confused by the arguments
    sys.argv = sys.argv[:1]

    return setup_configurable_test(
        args.threads,
        args.time,
        args.slack,
        args.interval,
        args.units,
    )


if __name__ == "__main__":
    the_test = main_parse_args_and_get_test()
    unittest.main()
