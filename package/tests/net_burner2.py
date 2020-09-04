#! /usr/bin/env python2
#
# Copyright (C) 2018-2020 CERN
# License Apache2 - see LICENCE file
#
# Request network data for prmon unittest (Python2 version)
from __future__ import print_function, unicode_literals

import argparse
import time
import urllib2


def getNetData(host="localhost", port="8000", blocks=None):
    url = "http://" + host + ":" + str(port) + "/cgi-bin/http_block2.py"
    if blocks:
        url += "?blocks=" + str(blocks)
    response = urllib2.urlopen(url)
    html = response.read()
    print("Read {0} bytes".format(len(html)))
    return len(html)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Network data burner")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--blocks", type=int)
    parser.add_argument("--requests", type=int, default=10)
    parser.add_argument("--sleep", type=float, default=0.1)
    parser.add_argument("--pause", type=float, default=3)
    args = parser.parse_args()

    time.sleep(args.pause)

    readBytes = 0
    for req in range(args.requests):
        time.sleep(args.sleep)
        readBytes += getNetData(args.host, args.port, args.blocks)

    print("Read total of {0} bytes in {1} requests".format(readBytes, args.requests))
    time.sleep(args.pause)
