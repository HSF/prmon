#! /usr/bin/env python
#
# Copyright (C) CERN, 2018
#
# Test script for network IO
from __future__ import print_function, unicode_literals

import argparse
import json
import os
import signal
import subprocess
import sys
import unittest

def setupConfigurableTest(blocks=None, requests=10, sleep=None, pause=None, slack=0.95):
    '''Wrap the class definition in a function to allow arguments to be passed'''
    class configurableProcessMonitor(unittest.TestCase):
        def setUp(self):
            # Start the simple python HTTP CGI server
            httpServerCmd = ['python', '-m', 'CGIHTTPServer']
            self.httpServer = subprocess.Popen(httpServerCmd, shell = False)
            
        def tearDown(self):
            os.kill(self.httpServer.pid, signal.SIGTERM)

        def test_runTestWithParams(self):
            burn_cmd = ['python', './netBurner.py']
            if (requests):
                burn_cmd.extend(['--requests', str(requests)])
            if (pause):
                burn_cmd.extend(['--pause', str(pause)])
            if (sleep):
                burn_cmd.extend(['--sleep', str(sleep)])
            if blocks:
                burn_cmd.extend(['--blocks', str(blocks)])
            burn_p = subprocess.Popen(burn_cmd, shell=False)    
            prmon_cmd = ['../prmon', '--pid', str(burn_p.pid)]
            prmon_p = subprocess.Popen(prmon_cmd, shell = False)
    
            burn_rc = burn_p.wait()
            prmon_rc = prmon_p.wait()
    
            self.assertEqual(prmon_rc, 0, "Non-zero return code from prmon")
            prmonJSON = json.load(open("prmon.json"))
            expectedBytes = 1025000 * requests * slack;
            self.assertGreater(prmonJSON["Max"]["rx_bytes"], expectedBytes, "Too low value for rx bytes "
                               "(expected minimum of {0}, got {1})".format(expectedBytes, prmonJSON["Max"]["rx_bytes"]))
            self.assertGreater(prmonJSON["Max"]["tx_bytes"], expectedBytes, "Too low value for tx bytes "
                               "(expected minimum of {0}, got {1})".format(expectedBytes, prmonJSON["Max"]["tx_bytes"]))
    
    return configurableProcessMonitor


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Configurable test runner for network access")
    parser.add_argument('--blocks', type=int)
    parser.add_argument('--requests', type=int, default=10)
    parser.add_argument('--sleep', type=float)
    parser.add_argument('--pause', type=float)
    parser.add_argument('--slack', type=float, default=0.95)
    args = parser.parse_args()
    # Stop unittest from being confused by the arguments
    sys.argv=sys.argv[:1]
    
    cpm = setupConfigurableTest(args.blocks, args.requests, args.sleep, args.pause, args.slack)
    
    unittest.main()
