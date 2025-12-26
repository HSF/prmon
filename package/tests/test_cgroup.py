#! /usr/bin/env python3
#
# Copyright (C) 2018-2025 CERN
# License Apache2 - see LICENCE file

"""Test harness for cgroup monitoring"""

import argparse
import json
import os
import subprocess
import sys
import unittest


def setup_configurable_test(
    time=10.0,
    interval=1,
    invoke=False,
    units=False,
):
    """Wrap the class definition in a function to allow arguments to be passed"""

    class ConfigurableCgroupMonitor(unittest.TestCase):
        """Test class for cgroup monitoring"""

        def test_run_cgroup_test(self):
            """Test cgroup monitoring functionality"""

            # Check if running in a container/cgroup environment
            has_cgroup_v2 = os.path.exists("/sys/fs/cgroup/cgroup.controllers")
            has_cgroup_v1 = os.path.exists("/sys/fs/cgroup/cpu")

            if not (has_cgroup_v2 or has_cgroup_v1):
                self.skipTest("No cgroup support detected on this system")

            burn_cmd = ["./burner", "--time", str(time)]

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
                burn_p.wait()
                prmon_rc = prmon_p.wait()

            self.assertEqual(prmon_rc, 0, "Non-zero return code from prmon")

            with open("prmon.json") as infile:
                prmon_json = json.load(infile)

                # Check that cgroup stats exist if cgroups are available
                if has_cgroup_v2 or has_cgroup_v1:
                    # Check for at least some cgroup metrics
                    cgroup_metrics_found = False
                    for key in prmon_json.get("Max", {}).keys():
                        if key.startswith("cgroup_"):
                            cgroup_metrics_found = True
                            break

                    # Note: cgroupmon might be disabled or invalid,
                    # so this is informational
                    if cgroup_metrics_found:
                        print("Cgroup metrics detected in output")

                        # Validate that CPU metrics are reasonable
                        if "cgroup_cpu_total" in prmon_json["Max"]:
                            self.assertGreater(
                                prmon_json["Max"]["cgroup_cpu_total"],
                                0,
                                "Cgroup CPU time should be greater than 0",
                            )
                    else:
                        print(
                            "Note: Cgroup metrics not found (monitor may be inactive)"
                        )

                # Unit test
                if units:
                    for group in ("Max", "Avg"):
                        if group not in prmon_json:
                            continue
                        value_params = set(prmon_json[group].keys())
                        if "Units" in prmon_json and group in prmon_json["Units"]:
                            unit_params = set(prmon_json["Units"][group].keys())
                            cgroup_values = {
                                k for k in value_params if k.startswith("cgroup_")
                            }
                            cgroup_units = {
                                k for k in unit_params if k.startswith("cgroup_")
                            }

                            # Check that cgroup metrics have units
                            if cgroup_values:
                                missing = cgroup_values - cgroup_units
                                self.assertEqual(
                                    len(missing),
                                    0,
                                    (
                                        "Missing unit descriptions for cgroup "
                                        "parameters: "
                                        f"{missing}"
                                    ),
                                )

    return ConfigurableCgroupMonitor


def main():
    """Main test driver"""
    parser = argparse.ArgumentParser(description="Cgroup monitoring tests")
    parser.add_argument("--time", type=float, default=10.0, help="Burn time in seconds")
    parser.add_argument(
        "--interval", type=int, default=1, help="Monitoring interval in seconds"
    )
    parser.add_argument(
        "--invoke", action="store_true", help="Use prmon to invoke test"
    )
    parser.add_argument("--units", action="store_true", help="Test units output")

    args = parser.parse_args()

    suite = unittest.TestSuite()
    suite.addTest(
        setup_configurable_test(
            time=args.time,
            interval=args.interval,
            invoke=args.invoke,
            units=args.units,
        )("test_run_cgroup_test")
    )

    runner = unittest.TextTestRunner()
    result = runner.run(suite)

    sys.exit(not result.wasSuccessful())


if __name__ == "__main__":
    main()
