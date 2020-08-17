#! /usr/bin/env python3
#
# Copyright (C) 2018-2020 CERN
# License Apache2 - see LICENCE file

import argparse
import subprocess
import sys
import unittest

def setupConfigurableTest(exit_code = 0):
    '''Wrap the class definition in a function to allow arguments to be passed'''
    class configurableProcessMonitor(unittest.TestCase):
        def test_runTestWithParams(self):
            child_cmd = ['sh', '-c', 'sleep 3 && exit {0}'.format(exit_code)]

            prmon_cmd = ['../prmon', '--interval', '1']
            prmon_cmd.append('--')
            prmon_cmd.extend(child_cmd)
            prmon_p = subprocess.Popen(prmon_cmd, shell = False)
            prmon_rc = prmon_p.wait()
    
            self.assertEqual(prmon_rc, exit_code, "Wrong return code from prmon (expected {0}".format(exit_code))
    
    return configurableProcessMonitor


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Configurable test runner")
    parser.add_argument('--exit-code', type=int, default=0)

    args = parser.parse_args()
    # Stop unittest from being confused by the arguments
    sys.argv=sys.argv[:1]
    
    cpm = setupConfigurableTest(args.exit_code)
    
    unittest.main()
