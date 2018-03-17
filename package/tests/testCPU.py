#! /usr/bin/env python
#
# Probably we are python2, but use python3 syntax as much as possible
from __future__ import print_function, unicode_literals

import argparse
import json
import os
import subprocess
import sys
import unittest

def setupConfigurableTest(threads=1, procs=1, time=10, slack=0.75):
    '''Wrap the class definition in a function to allow arguments to be passed'''
    class configurableProcessMonitor(unittest.TestCase):
        def test_runTestWithParams(self):
            burn_cmd = ['./burner', '--time', str(time)]
            if threads != 1:
                burn_cmd.extend(['--threads', str(threads)])
            if procs != 1:
                burn_cmd.extend(['--procs', str(procs)])
            burn_p = subprocess.Popen(burn_cmd, shell = False)
    
            prmon_cmd = ['../prmon', '--pid', str(burn_p.pid)]
            prmon_p = subprocess.Popen(prmon_cmd, shell = False)
    
            burn_rc = burn_p.wait()
            prmon_rc = prmon_p.wait()
    
            self.assertEqual(prmon_rc, 0, "Non-zero return code from prmon")
            prmonJSON = json.load(open("prmon.json"))
            totCPU = prmonJSON["Max"]["totUTIME"] + prmonJSON["Max"]["totSTIME"]
            self.assertLess(totCPU, time*threads*procs, "Too high value for CPU time "
                            "(expected maximum of {0}, got {1})".format(time*threads*procs, totCPU))
            self.assertGreater(totCPU, time*threads*procs*slack, "Too low value for CPU time "
                               "(expected minimum of {0}, got {1}".format(time*threads*procs*slack, totCPU))
    
    return configurableProcessMonitor


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Configurable test runner")
    parser.add_argument('--threads', type=int, default=1)
    parser.add_argument('--procs', type=int, default=1)
    parser.add_argument('--time', type=float, default=10)
    parser.add_argument('--slack', type=float, default=0.75)
    args = parser.parse_args()
    # Stop unittest from being confused by the arguments
    sys.argv=sys.argv[:1]
    
    cpm = setupConfigurableTest(args.threads,args.procs,args.time,args.slack)
    
    unittest.main()
