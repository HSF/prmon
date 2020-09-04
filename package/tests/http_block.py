#! /usr/bin/env python3
#
# Copyright (C) 2018-2020 CERN
# License Apache2 - see LICENCE file

"""Simple CGI script that delivers a known block
   of data to the caller over HTTP

   One GET/POST parameter is recognised, which is "blocks"
   that specifies how many  1KB blocks are returned to the
   client (defaults to 1000, thus ~1MB delivered)"""

import cgi
import cgitb
import sys


def main():
    """main function"""
    cgitb.enable()

    print("Content-Type: text/plain\n")

    form = cgi.FieldStorage()
    blocks = form.getfirst("blocks", default="1000")
    try:
        blocks = int(blocks)
    except ValueError:
        print("Invalid block value")
        sys.exit(1)

    my_string = "somehow the world seems more curious than when i was a child xx\n"
    my_block = my_string * 16  # 1KB

    for _ in range(blocks):
        print(my_block)


if "__main__" in __name__:
    main()
