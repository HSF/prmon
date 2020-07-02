#! /usr/bin/env python3
#
# Copyright (C) 2018-2020 CERN
# License Apache2 - see LICENCE file

import argparse
import json
import os
import subprocess
import sys
import unittest

def setupConfigurableTest(threads=1, procs=1, child_fraction=1.0, time=10.0, 
    slack=0.75, interval=1, invoke=False, units=False):
    '''Wrap the class definition in a function to allow arguments to be passed'''
    class configurableProcessMonitor(unittest.TestCase):
        def test_runTestWithParams(self):
            burn_cmd = ['./burner', '--time', str(time)]
            if threads != 1:
                burn_cmd.extend(['--threads', str(threads)])
            if procs != 1:
                burn_cmd.extend(['--procs', str(procs)])
            if child_fraction != 1.0:
                burn_cmd.extend(['--child-fraction', str(child_fraction)])

            prmon_cmd = ['../prmon', '--interval', str(interval)]
            if units:
                prmon_cmd.append('--units')
            if invoke:
                prmon_cmd.append('--')
                prmon_cmd.extend(burn_cmd)
                prmon_p = subprocess.Popen(prmon_cmd, shell = False)

                prmon_rc = prmon_p.wait()
            else:
                burn_p = subprocess.Popen(burn_cmd, shell = False)
    
                prmon_cmd.extend(['--pid', str(burn_p.pid)])
                prmon_p = subprocess.Popen(prmon_cmd, shell = False)
    
                burn_rc = burn_p.wait()
                prmon_rc = prmon_p.wait()
    
            self.assertEqual(prmon_rc, 0, "Non-zero return code from prmon")

            with open("prmon.json") as infile:
                prmonJSON = json.load(infile)

                # CPU time tests
                totCPU = prmonJSON["Max"]["utime"] + prmonJSON["Max"]["stime"]
                expectCPU = (1.0 + (procs-1)*child_fraction) * time * threads
                self.assertLessEqual(totCPU, expectCPU, "Too high value for CPU time "
                                "(expected maximum of {0}, got {1})".format(expectCPU, totCPU))
                self.assertGreaterEqual(totCPU, expectCPU*slack, "Too low value for CPU time "
                                   "(expected minimum of {0}, got {1}".format(expectCPU*slack, totCPU))
                # Wall time tests
                totWALL = prmonJSON["Max"]["wtime"]
                self.assertLessEqual(totWALL, time, "Too high value for wall time "
                                "(expected maximum of {0}, got {1})".format(time, totWALL))
                self.assertGreaterEqual(totWALL, time*slack, "Too low value for wall time "
                                   "(expected minimum of {0}, got {1}".format(time*slack, totWALL))

                # Unit test
                if units:
                    for group in ("Max", "Avg"):
                        value_params = set(prmonJSON[group].keys())
                        unit_params = set(prmonJSON["Units"][group].keys())
                        missing = value_params - unit_params
                        self.assertEqual(len(missing), 0, 
                            "Wrong number of unit values for '{0}' - missing parameters are {1}".format(group, missing))
                        extras = unit_params - value_params
                        self.assertEqual(len(extras), 0, 
                            "Wrong number of unit values for '{0}' - extra parameters are {1}".format(group, extras))
    
    return configurableProcessMonitor


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Configurable test runner")
    parser.add_argument('--threads', type=int, default=1)
    parser.add_argument('--procs', type=int, default=1)
    parser.add_argument('--child-fraction', type=float, default=1.0)
    parser.add_argument('--time', type=float, default=10)
    parser.add_argument('--slack', type=float, default=0.7)
    parser.add_argument('--interval', type=int, default=1)
    parser.add_argument('--invoke', action='store_true')
    parser.add_argument('--units', action='store_true')

    args = parser.parse_args()
    # Stop unittest from being confused by the arguments
    sys.argv=sys.argv[:1]
    
    cpm = setupConfigurableTest(args.threads,args.procs,args.child_fraction,args.time,
        args.slack,args.interval,args.invoke,args.units)
    
    unittest.main()
