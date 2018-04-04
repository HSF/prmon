#! /usr/bin/env python

import argparse
import sys
import os
try:
    import pandas as pd
    import matplotlib as mpl
    mpl.use('Agg')
    import matplotlib.pyplot as plt
    import matplotlib.ticker as ticker
    from matplotlib.dates import DateFormatter
    plt.style.use('ggplot')
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
    parser.add_argument('--yvar', type = str, default = 'PSS', 
                        help = 'name of the variable to be plotted in the y-axis')
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
    if args.yvar not in list(data):
        print('Variable %s is not available in data'%(args.yvar))
        sys.exit(-1)

    # Make the plot and save - vey basic, nothing fancy here
    xlabel = args.xvar
    ylabel = args.yvar
    output = 'PrMon_%s_vs_%s.png'%(args.xvar,args.yvar)

    fig, ax1 = plt.subplots()
    plt.plot(data[args.xvar], data[args.yvar], lw = 2, label = args.yvar)
    plt.legend(loc=1)
    if 'Time' in args.xvar:
        formatter = DateFormatter('%H:%M:%S')
        ax1.xaxis.set_major_formatter(formatter)
    if args.xvar in ['VMEM', 'PSS', 'RSS', 'Swap']:
        xlabel += ' [kb]'
    elif args.xvar in ['utime', 'stime', 'cutime', 'cstime', 'wtime']:
        xlabel += ' [s]'
    if args.yvar in ['VMEM', 'PSS', 'RSS', 'Swap']:
        ylabel += ' [kb]'
    elif args.yvar in ['utime', 'stime', 'cutime', 'cstime', 'wtime']:
        ylabel += ' [s]'
    plt.title('Plot of %s vs %s obtained from PrMon output'%(args.xvar, args.yvar))
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    fig.savefig(output)

    print('Saved output into %s'%(output))
    sys.exit(0)
