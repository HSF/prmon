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

if '__main__' in __name__:
    # Parse the user input
    parser = argparse.ArgumentParser(description = 'Configurable plotting script')
    parser.add_argument('--input', type = str, default = 'prmon.txt', 
                        help = 'PrMon TXT output that will be used as input'     )
    parser.add_argument('--xvar', type = str, default = 'Time', 
                        help = 'name of the variable to be plotted in the x-axis')
    parser.add_argument('--yvar', type = str, default = 'pss', 
                        help = 'name(s) of the variable to be plotted in the y-axis' 
                               ' (comma seperated list is accepted)')
    parser.add_argument('--stacked', dest = 'stacked', action = 'store_true',
                        help = 'stack plots if specified')
    parser.set_defaults(stacked = False)
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

    # Make the plot and save - vey basic, nothing fancy here
    xlabel = args.xvar
    ylabel = ''
    for carg in ylist:
        if ylabel: ylabel += '_'
        ylabel += carg.lower()
    output = 'PrMon_%s_vs_%s.png'%(xlabel,ylabel)

    fig, ax1 = plt.subplots()
    xdata = np.array(data[xlabel])
    ydlist = []
    for carg in ylist:
        ydlist.append(np.array(data[carg]))
    if args.stacked:
        ydata = np.vstack(ydlist)
        plt.stackplot(xdata, ydata, lw = 2, labels = ylist, alpha = 0.6) 
    else:
        for cidx,cdata in enumerate(ydlist):
            plt.plot(xdata, cdata, lw = 2, label = ylist[cidx]) 
    plt.legend(loc=2)
    if 'Time' in xlabel:
        formatter = DateFormatter('%H:%M:%S')
        ax1.xaxis.set_major_formatter(formatter)
    plt.title('Plot of %s vs %s obtained from PrMon output'%(xlabel, ylabel))
    if 'vmem' in xlabel or 'pss' in xlabel or 'rss' in xlabel:
        xlabel += ' [kb]'
    elif 'utime' in xlabel or 'stime' in xlabel or 'wtime' in xlabel:
        xlabel += ' [s]'
    if 'vmem' in ylabel or 'pss' in ylabel or 'rss' in ylabel:
        ylabel += ' [kb]'
    elif 'utime' in ylabel or 'stime' in ylabel or 'wtime' in ylabel:
        ylabel += ' [s]'
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    fig.savefig(output)

    print('Saved output into %s'%(output))
    sys.exit(0)
