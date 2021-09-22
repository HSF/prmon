#! /usr/bin/env python3
"""prmon output smart compression script"""

import argparse
import os
import sys

try:
    import pandas as pd
except ImportError:
    print("ERROR:: This script needs pandas.")
    sys.exit(-1)


MEMORY_IO_NETWORK_GPU_CPU = [
    "vmem",
    "pss",
    "rss",
    "swap",
    "rchar",
    "wchar",
    "read_bytes",
    "write_bytes",
    "rx_packets",
    "tx_packets",
    "rx_bytes",
    "tx_bytes",
    "gpufbmem",
    "gpumempct",
    "gpusmpct",
    "utime",
    "stime",
]

NPROCS_NTHREADS_NGPUS = ["nprocs", "nthreads", "ngpus"]


def interp_drop(p1, p2, p3, eps):
    """Computesinterpolation and checks if middle point falls within threshold"""
    t = p1[1] + (p3[1] - p1[1]) / (p3[0] - p1[0]) * (p2[0] - p1[0])
    return abs(t - p2[1]) < eps


def reduce_changing_metric(df, metric, precision):
    """Iteratively compress metric"""
    metric_series = df[metric]
    metric_redux = metric_series.copy()
    dyn_range = metric_series.max() - metric_series.min()
    eps = dyn_range * precision
    idx = 0
    while True:
        metriclen = len(metric_redux)
        if idx == metriclen - 2:
            break
        p1 = (metric_redux.index[idx], metric_redux.iloc[idx])
        p2 = (metric_redux.index[idx + 1], metric_redux.iloc[idx + 1])
        p3 = (metric_redux.index[idx + 2], metric_redux.iloc[idx + 2])
        if interp_drop(p1, p2, p3, eps):
            metric_redux = metric_redux.drop(metric_redux.index[idx + 1])
        else:
            idx += 1
    return metric_redux


def reduce_steady_metric(df, metric):
    """For more steady metrics just keep the changing points"""
    metric = df[metric]
    return metric[metric != metric.shift(1)]


def compress_prmon_output(df, precision, skip_interpolate):
    """Compress full df. Final index is the union of the compressed series indexes.
    Points without values for a series are either linearly interpolated,
    for fast-changing metrics, or forward-filled, for steady metrics"""
    if len(df) > 2:
        present_changing_metrics = [
            metric for metric in MEMORY_IO_NETWORK_GPU_CPU if metric in df.columns
        ]
        present_steady_metrics = [
            metric for metric in NPROCS_NTHREADS_NGPUS if metric in df.columns
        ]
        reduced_changing_metrics = [
            reduce_changing_metric(df, metric, precision)
            for metric in present_changing_metrics
        ]
        reduced_steady_metrics = [
            reduce_steady_metric(df, metric) for metric in present_steady_metrics
        ]
        final_df = pd.concat(reduced_changing_metrics + reduced_steady_metrics, axis=1)
        if not skip_interpolate:
            final_df[present_changing_metrics] = final_df[
                present_changing_metrics
            ].interpolate(method="index")
            final_df[present_steady_metrics] = final_df[present_steady_metrics].ffill(
                downcast="infer"
            )
            final_df = final_df.round(0)
        final_df = final_df.astype("Int64", errors="ignore")
        return final_df
    return df


def main():
    """Main compression function"""
    parser = argparse.ArgumentParser(
        description="Configurable smart compression script"
    )

    parser.add_argument(
        "--input",
        type=str,
        default="prmon.txt",
        help="PrMon TXT output that will be used as input",
    )

    parser.add_argument(
        "--output",
        type=str,
        default="prmon_compressed.txt",
        help="name of the output compressed text file",
    )

    parser.add_argument(
        "--precision",
        type=lambda x: float(x)
        if 0 < float(x) < 1
        else parser.exit(-1, "Precision must be strictly between 0 and 1"),
        default=0.05,
        help="precision value for interpolation threshold",
    )

    parser.add_argument(
        "--skip-interpolate",
        default=False,
        action="store_true",
        help="""Whether to skip interpolation of the final obtained df,
                and leave NAs for the different metrics""",
    )

    parser.add_argument(
        "--delete-original",
        default=False,
        action="store_true",
        help="""Add this to delete the original, uncompressed
                file""",
    )

    args = parser.parse_args()

    df = pd.read_csv(
        args.input, sep="\t", index_col="Time", engine="c", na_filter=False
    )
    compressed_df = compress_prmon_output(df, args.precision, args.skip_interpolate)
    compressed_df["wtime"] = df[df.index.isin(compressed_df.index)]["wtime"]
    compressed_df.to_csv(args.output, sep="\t")
    if args.delete_original:
        os.remove(args.input)


if "__main__" in __name__:
    main()
