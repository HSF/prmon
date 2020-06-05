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

def setupConfigurableTest(threads=1, procs=1, time=10.0, interval=1, invoke=False):
    '''Wrap the class definition in a function to allow arguments to be passed'''
    class configurableProcessMonitor(unittest.TestCase):
        def test_runTestWithParams(self):
            burn_cmd = ['./burner', '--time', str(time)]
            if threads != 1:
                burn_cmd.extend(['--threads', str(threads)])
            if procs != 1:
                burn_cmd.extend(['--procs', str(procs)])

            prmon_cmd = ['../prmon', '--interval', str(interval)]
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

                # Simple Process and Thread counting test 
                totPROC      = prmonJSON["Max"]["nprocs"] 
                totTHREAD    = prmonJSON["Max"]["nthreads"]
                expectPROC   = procs
                expectTHREAD = procs*threads
                self.assertAlmostEqual(totPROC, expectPROC, msg = "Inconsistent value for number of processes "
                             "(expected {0}, got {1})".format(expectPROC, totPROC))
                self.assertAlmostEqual(totTHREAD, expectTHREAD, msg = "Inconsistent value for number of total threads "
                                "(expected {0}, got {1}".format(expectTHREAD, totTHREAD))
                    
    return configurableProcessMonitor


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Configurable test runner")
    parser.add_argument('--threads', type=int, default=1)
    parser.add_argument('--procs', type=int, default=1)
    parser.add_argument('--time', type=float, default=10)
    parser.add_argument('--interval', type=int, default=1)
    parser.add_argument('--invoke', dest='invoke', action='store_true', default=False)

    args = parser.parse_args()
    # Stop unittest from being confused by the arguments
    sys.argv=sys.argv[:1]
    
    cpm = setupConfigurableTest(args.threads,args.procs,args.time,args.interval,args.invoke)
    
    unittest.main()
