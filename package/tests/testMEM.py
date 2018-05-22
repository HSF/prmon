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


def setupConfigurableTest(procs=4, malloc_mb = 100, write_fraction=0.5, sleep=10, slack=0.9):
    '''Wrap the class definition in a function to allow arguments to be passed'''
    class configurableProcessMonitor(unittest.TestCase):
        def checkMemoryLimits(self, name, value, expected, slack):
            max_value = expected * (1.0 + (1.0-slack))
            min_value = expected * slack
            self.assertLess(value, max_value, "Too high a value for {0} "
                                    "(expected maximum of {1}, got {2})".format(name, max_value, value))
            self.assertGreater(value, min_value, "Too low a value for {0} "
                                    "(expected maximum of {1}, got {2})".format(name, min_value, value))

        def test_runTestWithParams(self):
            burn_cmd = ['./mem-burner', '--sleep', str(sleep), '--malloc', str(malloc_mb), '--writef', str(write_fraction)]
            if procs != 1:
                burn_cmd.extend(['--procs', str(procs)])

            prmon_cmd = ['../prmon', '--']
            prmon_cmd.extend(burn_cmd)
            prmon_p = subprocess.Popen(prmon_cmd, shell = False)

            prmon_rc = prmon_p.wait()
    
            self.assertEqual(prmon_rc, 0, "Non-zero return code from prmon")
            prmonJSON = json.load(open("prmon.json"))
            # Memory tests
            vmemExpect = malloc_mb * procs * 1024 + 10000 * procs # Small uplift for program itself
            self.checkMemoryLimits("vmem", prmonJSON["Max"]["vmem"], vmemExpect, slack)

            rssExpect = malloc_mb * procs * 1024 * write_fraction
            self.checkMemoryLimits("rss", prmonJSON["Max"]["rss"], rssExpect, slack)

            pssExpect = malloc_mb * 1024 * write_fraction
            self.checkMemoryLimits("pss", prmonJSON["Max"]["pss"], pssExpect, slack)

    
    return configurableProcessMonitor


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Configurable memory test runner")
    parser.add_argument('--procs', type=int, default=4)
    parser.add_argument('--malloc', type=int, default=100)
    parser.add_argument('--writef', type=float, default=0.5)
    parser.add_argument('--sleep', type=int, default=10)
    parser.add_argument('--slack', type=float, default=0.9)

    args = parser.parse_args()
    # Stop unittest from being confused by the arguments
    sys.argv=sys.argv[:1]
    
    cpm = setupConfigurableTest(args.procs,args.malloc,args.writef,args.sleep,args.slack)
    
    unittest.main()
