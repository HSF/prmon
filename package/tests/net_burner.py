#! /usr/bin/env python3
#
# Copyright (C) 2018-2020 CERN
# License Apache2 - see LICENCE file

"""Request network data for prmon unittest"""

import argparse
import time
import urllib.request


def get_net_data(host="localhost", port="8000", blocks=None):
    """read network data from local http server"""
    url = "http://" + host + ":" + str(port) + "/cgi-bin/http_block.py"
    if blocks:
        url += "?blocks=" + str(blocks)
    # This URL is fine as it's from the server we setup
    response = urllib.request.urlopen(url)  # nosec
    html = response.read()
    print("Read {0} bytes".format(len(html)))
    return len(html)


def main():
    """parse arguments and read data"""
    parser = argparse.ArgumentParser(description="Network data burner")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--blocks", type=int)
    parser.add_argument("--requests", type=int, default=10)
    parser.add_argument("--sleep", type=float, default=0.1)
    parser.add_argument("--pause", type=float, default=3)
    args = parser.parse_args()

    time.sleep(args.pause)

    read_bytes = 0
    for _ in range(args.requests):
        time.sleep(args.sleep)
        read_bytes += get_net_data(args.host, args.port, args.blocks)

    print("Read total of {0} bytes in {1} requests".format(read_bytes, args.requests))

    time.sleep(args.pause)


if __name__ == "__main__":
    main()
