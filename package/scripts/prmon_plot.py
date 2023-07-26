#! /usr/bin/env python3
"""prmon output data plotting script"""

import argparse
import sys
import os

try:
    import pandas as pd
    import numpy as np
    import matplotlib as mpl

    mpl.use("Agg")
    import matplotlib.pyplot as plt

    plt.style.use("seaborn-whitegrid")
except ImportError:
    print("ERROR:: This script needs numpy, pandas and matplotlib.")
    print("ERROR:: Looks like at least one of these modules is missing.")
    print("ERROR:: Please install them first and then retry.")
    sys.exit(-1)

# Define the labels/units for beautification
# First allowed unit is the default
ALLOWEDUNITS = {
    "vmem": ["kb", "b", "mb", "gb"],
    "pss": ["kb", "b", "mb", "gb"],
    "rss": ["kb", "b", "mb", "gb"],
    "swap": ["kb", "b", "mb", "gb"],
    "utime": ["sec", "min", "hour"],
    "stime": ["sec", "min", "hour"],
    "wtime": ["sec", "min", "hour"],
    "rchar": ["b", "kb", "mb", "gb"],
    "wchar": ["b", "kb", "mb", "gb"],
    "read_bytes": ["b", "kb", "mb", "gb"],
    "write_bytes": ["b", "kb", "mb", "gb"],
    "rx_packets": ["1"],
    "tx_packets": ["1"],
    "rx_bytes": ["b", "kb", "mb", "gb"],
    "tx_bytes": ["b", "kb", "mb", "gb"],
    "nprocs": ["1"],
    "nthreads": ["1"],
    "gpufbmem": ["kb", "b", "mb", "gb"],
    "gpumempct": ["%"],
    "gpusmpct": ["%"],
    "ngpus": ["1"],
}

AXISNAME = {
    "vmem": "Memory",
    "pss": "Memory",
    "rss": "Memory",
    "swap": "Memory",
    "utime": "CPU-time",
    "stime": "CPU-time",
    "wtime": "Wall-time",
    "rchar": "I/O",
    "wchar": "I/O",
    "read_bytes": "I/O",
    "write_bytes": "I/O",
    "rx_packets": "Network",
    "tx_packets": "Network",
    "rx_bytes": "Network",
    "tx_bytes": "Network",
    "nprocs": "Count",
    "nthreads": "Count",
    "gpufbmem": "Memory",
    "gpumempct": "Memory",
    "gpusmpct": "Streaming Multiprocessors",
    "ngpus": "Count",
}

LEGENDNAMES = {
    "vmem": "Virtual Memory",
    "pss": "Proportional Set Size",
    "rss": "Resident Set Size",
    "swap": "Swap Size",
    "utime": "User CPU-time",
    "stime": "System CPU-time",
    "wtime": "Wall-time",
    "rchar": "I/O Read (rchar)",
    "wchar": "I/O Written (wchar)",
    "read_bytes": "I/O Read (read_bytes)",
    "write_bytes": "I/O Written (write_bytes)",
    "rx_packets": "Network Received (packets)",
    "tx_packets": "Network Transmitted (packets)",
    "rx_bytes": "Network Received (bytes)",
    "tx_bytes": "Network Transmitted (bytes)",
    "nprocs": "Number of Processes",
    "nthreads": "Number of Threads",
    "gpufbmem": "GPU Memory",
    "gpumempct": "GPU Memory",
    "gpusmpct": "GPU Streaming Multiprocessors",
    "ngpus": "Number of GPUs",
}

MULTIPLIERS = {
    "SEC": 1.0,
    "MIN": 60.0,
    "HOUR": 60.0 * 60.0,
    "B": 1.0,
    "KB": 1024.0,
    "MB": 1024.0 * 1024.0,
    "GB": 1024.0 * 1024.0 * 1024.0,
    "1": 1.0,
    "%": 1.0,
}


# A few basic functions for labels and conversions
def get_axis_label(nom, denom=None):
    """Generate axis label from variable and units"""
    label = AXISNAME[nom]
    if denom:
        label = r"$\Delta$" + label + r"/$\Delta$" + AXISNAME[denom]
    return label


def get_multiplier(label, unit):
    """Get the multiplication constant for a label"""
    return MULTIPLIERS[ALLOWEDUNITS[label][0].upper()] / MULTIPLIERS[unit]


# Function for checking the input file exists

def check_input_file(file):
    if not os.path.exists(file):
        print(f"ERROR:: Input file {file} does not exist")
        sys.exit(-1)


# Function for loading the data

def load_data(file):
    data = pd.read_csv(file, sep="\t")
    data["Time"] = pd.to_datetime(data["Time"], unit="s")
    return data


# Function for checking whether the variables are in the data

def check_variables(data, var, ylist):
    if var not in list(data):
        print(f"ERROR:: Variable {var} is not available in one of the data sets")
        sys.exit(-1)
    for carg in ylist:
        if carg not in list(data):
            print(f"ERROR:: Variable {carg} is not available in one of the data sets")


# This function creates the final data set of y-values

def create_list(ylist, data, args, xmult, ymult, xlabel):
    ydlist = []
    for carg in ylist:
        if args.diff:
            num = np.array(data[carg].diff()) * ymult
            denom = np.array(data[xlabel].diff()) * xmult
            ratio = np.where(denom != 0, num / denom, np.nan)
            ydlist.append(ratio)
        else:
            ydlist.append(np.array(data[carg]) * ymult)
    return ydlist

# Graph plotting functions

def draw_stacked_graph(xdata, ydlist, ylist):
    ydata = np.vstack(ydlist)
    plt.stackplot(
        xdata, ydata, lw=2, labels=[LEGENDNAMES[val] for val in ylist], alpha=0.6
    )

def draw_line_graph(xdata, ydlist, ylist, style):
    colours = plt.rcParams['axes.prop_cycle'].by_key()['color']     # This is a list of the matplotlib default colours
    count = 0
    for cidx, cdata in enumerate(ydlist):
        plt.plot(xdata, cdata, lw=2, label=LEGENDNAMES[ylist[cidx]], color=colours[count], linestyle=style)
        count += 1


# Main function

def main():
    """prmon plotting main function"""

    # Default xvar, xunit, yvar, and yunit
    default_xvar, default_xunit = "wtime", ALLOWEDUNITS["wtime"][0].upper()
    default_yvar, default_yunit = "pss", ALLOWEDUNITS["pss"][0].upper()

    # Parse the user input
    parser = argparse.ArgumentParser(description="Configurable plotting script")
    parser.add_argument(
        "--input",
        type=str,
        default="prmon.txt",
        help="First PrMon TXT output that will be used as input",
    )
    parser.add_argument(
        "--output",
        type=str,
        default=None,
        help="name of the output image without the file extension",
    )
    parser.add_argument(
        "--xvar",
        type=str,
        default=default_xvar,
        help="name of the variable to be plotted in the x-axis",
    )
    parser.add_argument(
        "--xunit",
        nargs="?",
        default=default_xunit,
        choices=["SEC", "MIN", "HOUR", "B", "KB", "MB", "GB", "1", "%"],
        help="unit of the variable to be plotted in the x-axis",
    )
    parser.add_argument(
        "--yvar",
        type=str,
        default=default_yvar,
        help="name(s) of the variable(s) to be plotted in the y-axis"
        " (comma seperated list is accepted)",
    )
    parser.add_argument(
        "--yunit",
        nargs="?",
        default=default_yunit,
        choices=["SEC", "MIN", "HOUR", "B", "KB", "MB", "GB", "1", "%"],
        help="unit of the variable(s) to be plotted in the y-axis",
    )
    parser.add_argument(
        "--stacked",
        dest="stacked",
        action="store_true",
        help="stack plots if specified",
    )
    parser.add_argument(
        "--diff",
        dest="diff",
        action="store_true",
        help="plot the ratio of the discrete differences of "
        " the elements for yvars and xvars if specified (i.e. dy/dx)",
    )
    parser.add_argument(
        "--otype",
        nargs="?",
        default="png",
        choices=["png", "pdf", "svg"],
        help="format of the output image",
    )
    parser.set_defaults(stacked=False)
    parser.set_defaults(diff=False)
    args = parser.parse_args()

    inputs = args.input.split(",")
    ylist = args.yvar.split(",")
    data = []

    for i in range(len(inputs)):
        check_input_file(inputs[i])
        data.append(load_data(inputs[i]))
        check_variables(data[i], args.xvar, ylist)

    # Check the consistency of variables and units
    # If they don't match, reset the units to defaults
    first_x_variable = args.xvar.split(",")[0]
    if args.xunit.lower() not in ALLOWEDUNITS[first_x_variable]:
        old_xunit = args.xunit
        args.xunit = ALLOWEDUNITS[first_x_variable][0].upper()
        print(
            f"WARNING:: Changing xunit from {old_xunit} to {args.xunit} for consistency"
        )
    first_y_variable = args.yvar.split(",")[0]
    if args.yunit.lower() not in ALLOWEDUNITS[first_y_variable]:
        old_yunit = args.yunit
        args.yunit = ALLOWEDUNITS[first_y_variable][0].upper()
        print(
            f"WARNING:: Changing yunit from {old_yunit} to {args.yunit} for consistency"
        )

    # Check if the user is trying to plot variables with inconsistent units
    # If so simply print a message to warn them
    # TEST THIS LATER
    if len({ALLOWEDUNITS[i][0] for i in args.xvar.split(",")}) > 1:
        print("WARNING:: Elements in xvar have inconsistent units, beware!")

    if len({ALLOWEDUNITS[i][0] for i in args.yvar.split(",")}) > 1:
        print("WARNING:: Elements in yvar have inconsistent units, beware!")

    # Labels and output information
    xlabel = args.xvar
    ylabel = ""
    for carg in ylist:
        if ylabel:
            ylabel += "_"
        ylabel += carg.lower()
    if args.diff:
        ylabel = "diff_" + ylabel
    if not args.output:
        output = "PrMon_{}_vs_{}.{}".format(xlabel, ylabel, args.otype)
    else:
        output = "{}.{}".format(args.output, args.otype)

    # Calculate the multipliers
    xmultiplier = get_multiplier(xlabel, args.xunit)
    ymultiplier = get_multiplier(ylist[0], args.yunit)

    # Here comes the figure and data extraction
    fig, ax1 = plt.subplots()

    xdata = []
    ydlist = []

    for i in range(len(data)):
        xdata.append(np.array(data[i][xlabel] * xmultiplier))
        ydlist.append(create_list(ylist, data[i], args, xmultiplier, ymultiplier, xlabel))

    # Plots the graphs

    line_styles = list(mpl.lines.lineStyles.keys())

    if args.stacked:
        if len(ylist) == 1:
            for i in range(len(xdata)):
                draw_stacked_graph(xdata[i], ydlist[i], ylist)
        else:
            print("ERROR:: Stacked graphs are not supported for more than one data set")
            sys.exit(-1)
    else:
        for i in range(len(xdata)):
            draw_line_graph(xdata[i], ydlist[i], ylist, line_styles[i % 7])

    # Creates the key

    plt.legend(loc=0)
    if "Time" in xlabel:
        formatter = mpl.dates.DateFormatter("%H:%M:%S")
        ax1.xaxis.set_major_formatter(formatter)
    fxlabel = get_axis_label(xlabel)
    fxunit = args.xunit
    if args.diff:
        fylabel = get_axis_label(ylist[0], xlabel)
        if args.yunit == args.xunit:
            fyunit = "1"
        else:
            fyunit = args.yunit + "/" + args.xunit
    else:
        fylabel = get_axis_label(ylist[0])
        fyunit = args.yunit

    plt.title("Plot of {} vs {}".format(fxlabel, fylabel), y=1.05)
    plt.xlabel((fxlabel + " [" + fxunit + "]") if fxunit != "1" else fxlabel)
    plt.ylabel((fylabel + " [" + fyunit + "]") if fyunit != "1" else fylabel)
    plt.tight_layout()
    fig.savefig(output)

    print(f"INFO:: Saved output into {output}")
    sys.exit(0)

if "__main__" in __name__:
    main()
