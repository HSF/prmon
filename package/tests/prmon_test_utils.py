#! /usr/bin/env python3
#
# Copyright (C) 2018-2026 CERN
# License Apache2 - see LICENCE file

"""Common utilities for the prmon python test wrappers"""

import functools
import sys

PRMON_OUTPUT_FILES = ("prmon.json", "prmon.txt", "prmon.log")


def dump_prmon_outputs(files=PRMON_OUTPUT_FILES):
    """Print prmon output files, to help debug failures where the
    files are not accessible afterwards (e.g., in CI)"""
    for filename in files:
        print(f"===== {filename} =====", file=sys.stderr)
        try:
            with open(filename) as infile:
                print(infile.read(), file=sys.stderr)
        except OSError as err:
            print(f"Could not read {filename}: {err}", file=sys.stderr)


def dump_on_failure(test_method):
    """Decorator for test methods that dumps the prmon output files
    if the test fails (or errors), then re-raises the exception"""

    @functools.wraps(test_method)
    def wrapper(*args, **kwargs):
        try:
            return test_method(*args, **kwargs)
        except Exception:
            dump_prmon_outputs()
            raise

    return wrapper
