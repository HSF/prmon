#! /usr/bin/env python

import argparse
import sys
import os

try:
    import pandas as pd
    import numpy as np
    import matplotlib as mpl
    mpl.use('Agg')
    import matplotlib.pyplot as plt
    import matplotlib.ticker as ticker
    from matplotlib.dates import DateFormatter
    plt.style.use('seaborn-whitegrid')
except ImportError:
    print('This script needs pandas and mathplotlib.'          )
    print('Looks like at least one of these module is missing.')
    print('Please install these modules first and then retry. ')
    sys.exit(-1)

# Define the labels/units for beautification
axisunits = {'vmem':'kb', 'pss':'kb', 'rss':'kb',
             'utime':'sec', 'stime':'sec', 'wtime':'sec',
             'rchar':'char','wchar':'char',
             'read_bytes':'b','write_bytes':'b',
             'rx_packets':'packets', 'tx_packets':'packets',
             'rx_bytes':'b','tx_bytes':'b'}

axisnames = {'vmem':'Memory', 
             'pss':'Memory', 
             'rss':'Memory',
             'utime':'CPU-time',
             'stime':'CPU-time',
             'wtime':'Wall-time',
             'rchar':'I/O',
             'wchar':'I/O',
             'read_bytes':'I/O',
             'write_bytes':'I/O',
             'rx_packets':'Network',
             'tx_packets':'Network',
             'rx_bytes':'Network',
             'tx_bytes':'Network'}

legendnames = {'vmem':'Virtual Memory', 
               'pss':'Proportional Set Size', 
               'rss':'Resident Set Size',
               'utime':'User CPU-time',
               'stime':'System CPU-time',
               'wtime':'Wall-time',
               'rchar':'I/O Read',
               'wchar':'I/O Written',
               'read_bytes':'I/O Read',
               'write_bytes':'I/O Written',
               'rx_packets':'Network Received',
               'tx_packets':'Network Transmitted',
               'rx_bytes':'Network Received',
               'tx_bytes':'Network Transmitted'}

multipliers = {'SEC': 1.,
               'MIN': 60.,
               'HR': 60.*60.,
               'B': 1.,
               'KB': 1024.,
               'MB': 1024.*1024.,
               'GB': 1024.*1024.*1024.,
               'CHAR': 1.,
               'PACKETS' : 1.}

# A few basic functions for labels/ conversions
def get_axis_label(nom, denom = None):
    label = axisnames[nom]
    if denom:
        label = '$\Delta$'+label+'/$\Delta$'+axisnames[denom] 
    return label

def get_multiplier(label, unit):
    return multipliers[axisunits[label].upper()]/multipliers[unit]

# Main function
if '__main__' in __name__:

    # Parse the user input
    parser = argparse.ArgumentParser(description = 'Configurable plotting script')
    parser.add_argument('--input', type = str, default = 'prmon.txt', 
                        help = 'PrMon TXT output that will be used as input'     )
    parser.add_argument('--xvar', type = str, default = 'wtime', 
                        help = 'name of the variable to be plotted in the x-axis')
    parser.add_argument('--xunit', nargs = '?', default = 'SEC',
                        choices=['SEC', 'MIN', 'HR', 'B', 'KB', 'MB', 'GB'],
                        help = 'unit of the variable to be plotted in the x-axis')
    parser.add_argument('--yvar', type = str, default = 'pss',
                        help = 'name(s) of the variable to be plotted in the y-axis' 
                               ' (comma seperated list is accepted)')
    parser.add_argument('--yunit', nargs = '?', default = 'MB',
                        choices=['SEC', 'MIN', 'HR', 'B', 'KB', 'MB', 'GB'],
                        help = 'unit of the variable to be plotted in the y-axis')
    parser.add_argument('--stacked', dest = 'stacked', action = 'store_true',
                        help = 'stack plots if specified')
    parser.add_argument('--diff', dest = 'diff', action = 'store_true',
                        help = 'plot the ratio of the discrete differences of '
                        ' the elements for yvars and xvars if specified')
    parser.add_argument('--otype', nargs = '?', default = 'png',
                        choices=['png', 'pdf', 'svg'],
                        help = 'format of the output image')
    parser.set_defaults(stacked = False)
    parser.set_defaults(diff = False)
    args = parser.parse_args()

    # Check the input file exists
    if not os.path.exists(args.input):
        print('Input file %s does not exists'%(args.input))
        sys.exit(-1)

    # Load the data
    data = pd.read_table(args.input, sep = '\t')
    data['Time'] = pd.to_datetime(data['Time'], unit = 's')

    # Check the variables are in data
    if args.xvar not in list(data):
        print('Variable %s is not available in data'%(args.xvar))
        sys.exit(-1)
    ylist = args.yvar.split(',')
    for carg in ylist:
        if carg not in list(data):
            print('Variable %s is not available in data'%(carg))
            sys.exit(-1)

    # Labels and output information
    xlabel = args.xvar
    ylabel = ''
    for carg in ylist:
        if ylabel: ylabel += '_'
        ylabel += carg.lower()
    if args.diff: ylabel = 'diff_' + ylabel
    output = 'PrMon_%s_vs_%s.%s'%(xlabel,ylabel,args.otype)

    # Calculate the multipliers
    xmultiplier = get_multiplier(xlabel, args.xunit)
    ymultiplier = get_multiplier(ylist[0], args.yunit)

    # Here comes the figure and data extraction
    fig, ax1 = plt.subplots()
    xdata = np.array(data[xlabel])*xmultiplier
    ydlist = []
    for carg in ylist:
        if args.diff:
            num = np.array(data[carg].diff())*ymultiplier
            denom = np.array(data[xlabel].diff())*xmultiplier
            ratio = np.where(denom != 0, num/denom, np.nan)
            ydlist.append(ratio)
        else:
            ydlist.append(np.array(data[carg])*ymultiplier)
    if args.stacked:
        ydata = np.vstack(ydlist)
        plt.stackplot(xdata, ydata, lw = 2, labels = [legendnames[val] for val in ylist], alpha = 0.6) 
    else:
        for cidx,cdata in enumerate(ydlist):
            plt.plot(xdata, cdata, lw = 2, label = legendnames[ylist[cidx]]) 
    plt.legend(loc=0)
    if 'Time' in xlabel:
        formatter = DateFormatter('%H:%M:%S')
        ax1.xaxis.set_major_formatter(formatter)
    fxlabel = get_axis_label(xlabel)
    fxunit  = args.xunit
    if args.diff:
        fylabel = get_axis_label(ylist[0],xlabel)
        if args.yunit == args.xunit:
            fyunit = '1'
        else:
            fyunit  = args.yunit+'/'+args.xunit
    else:
        fylabel = get_axis_label(ylist[0])
        fyunit  = args.yunit
    plt.title('Plot of %s vs %s'%(fxlabel, fylabel), y = 1.05)
    plt.xlabel(fxlabel+' ['+fxunit+']')
    plt.ylabel(fylabel+' ['+fyunit+']')
    plt.tight_layout()
    fig.savefig(output)

    print('Saved output into %s'%(output))
    sys.exit(0)
