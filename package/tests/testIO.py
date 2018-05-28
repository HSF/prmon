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

def setupConfigurableTest(io=10, threads=1, procs=1, usleep=10, pause=1, slack=0.95):
    '''Wrap the class definition in a function to allow arguments to be passed'''
    class configurableProcessMonitor(unittest.TestCase):
        def test_runTestWithParams(self):
            burn_cmd = ['./io-burner', '--io', str(io)]
            burn_cmd.extend(['--threads', str(threads)])
            burn_cmd.extend(['--procs', str(procs)])
            burn_cmd.extend(['--usleep', str(usleep)])
            burn_cmd.extend(['--pause', str(pause)])
            burn_p = subprocess.Popen(burn_cmd, shell = False)
    
            prmon_cmd = ['../prmon', '--pid', str(burn_p.pid)]
            prmon_p = subprocess.Popen(prmon_cmd, shell = False)
    
            burn_rc = burn_p.wait()
            prmon_rc = prmon_p.wait()
    
            self.assertEqual(prmon_rc, 0, "Non-zero return code from prmon")

            with open("prmon.json") as infile:
                prmonJSON = json.load(infile)

                # IO tests
                expectedBytes = io*threads*procs*slack * 1.0e6
                self.assertGreater(prmonJSON["Max"]["wchar"], expectedBytes, "Too low value for IO bytes written "
                                   "(expected minimum of {0}, got {1})".format(expectedBytes, prmonJSON["Max"]["wchar"]))
                self.assertGreater(prmonJSON["Max"]["rchar"], expectedBytes, "Too low value for IO bytes read "
                                   "(expected minimum of {0}, got {1})".format(expectedBytes, prmonJSON["Max"]["rchar"]))
    
    return configurableProcessMonitor


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Configurable test runner")
    parser.add_argument('--threads', type=int, default=1)
    parser.add_argument('--procs', type=int, default=1)
    parser.add_argument('--io', type=int, default=10)
    parser.add_argument('--usleep', type=int, default=10)
    parser.add_argument('--pause', type=float, default=1)
    parser.add_argument('--slack', type=float, default=0.95)
    args = parser.parse_args()
    # Stop unittest from being confused by the arguments
    sys.argv=sys.argv[:1]
    
    cpm = setupConfigurableTest(args.io, args.threads, args.procs, args.usleep, args.pause, args.slack)
    
    unittest.main()
