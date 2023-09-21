#!/usr/bin/env python
from __future__ import print_function
import argparse
import sys, os, contextlib
import logging
import json
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import pathlib

logging.basicConfig(format="[%(levelname)s] %(message)s")

Metrics = [
    "cpu_time",
    "real_time",
    "bytes_per_second",
    "items_per_second",
    "iterations",
]
Transforms = {
    "": lambda x: x,
    "nanos_to_micros": lambda x: x / 1000,
    "nanos_to_millis": lambda x: x / 1000000
}

BarColours = ['b', 'g', 'r', 'y', 'm', 'c']

def parse_args():
    parser = argparse.ArgumentParser(description="Visualize google-benchmark output")
    parser.add_argument(
        "-f",
        metavar="FILE",
        type=argparse.FileType("r"),
        default=sys.stdin,
        dest="file",
        help="path to file containing the csv or json benchmark data",
    )
    parser.add_argument(
        "--metric",
        metavar="METRIC",
        choices=Metrics,
        default=Metrics[0],
        dest="metric",
        help="metric to plot on the y-axis [%s]" % ", ".join(Metrics),
    )
    parser.add_argument(
        "--transform",
        metavar="TRANSFORM",
        choices=Transforms.keys(),
        default="",
        help="transform to apply to the chosen metric [%s]"
        % ", ".join(list(Transforms)),
        dest="transform",
    )
    parser.add_argument("--plot_width", type=int, default=5, help="plot width")
    parser.add_argument("--plot_height", type=int, default=8, help="plot height")
    parser.add_argument("--xlabel", type=str, default="input size", help="label of the x-axis")
    parser.add_argument("--ylabel", type=str, default="nanoseconds", help="label of the y-axis")
    parser.add_argument("--logy", action="store_true", help="plot y-axis on a logarithmic scale")
    parser.add_argument("--output", type=str, default="", help="File name without extension as a base for saved graphs per test category")
    parser.add_argument("-q", dest="quiet", action="store_true", help="don't display graphs")

    args = parser.parse_args()
    
    if args.ylabel == "nanoseconds":
        if args.transform == "nanos_to_micros":
            args.ylabel = "microseconds"
        elif args.transform == "nanos_to_millis":
            args.ylabel = "milliseconds"
    
    return args

def parse_input_size(name):
    splits = name.split("/")
    if len(splits) == 1:
        return 1
    return int(splits[len(splits)-1])

def read_data(args):
    """Read and process dataframe using commandline args"""
    extension = pathlib.Path(args.file.name).suffix
    try:
        if extension == ".csv":
            data = pd.read_csv(args.file, usecols=["name", args.metric, "label", "category"])
        elif extension == ".json":
            json_data = json.load(args.file)
            data = pd.DataFrame(json_data["benchmarks"])
        else:
            logging.error("Unsupported file extension '{}'".format(extension))
            exit(1)
    except ValueError:
        logging.error(
            'Could not parse the benchmark data. Did you forget "--benchmark_format=[csv|json] when running the benchmark"?'
        )
        exit(1)
    data["input"] = data["name"].apply(parse_input_size)
    data[args.metric] = data[args.metric].apply(Transforms[args.transform])
    return data

def get_shifted_indices(indices, curr_index, num_bars, bar_width):
    half = num_bars / 2
    shift = (curr_index - half) * bar_width
    return [x+shift for x in indices.copy()]

def bar_plot_groups(category, label_groups, args, outFileName):
    fig = plt.figure()
    fig.set_size_inches(args.plot_width, args.plot_height)
    axes = fig.add_subplot()

    axes.set_title(category)
    axes.set_xlabel(args.xlabel)
    axes.set_ylabel(args.ylabel)

    num_bars = len(label_groups)
    bar_width = 0.8 / num_bars
    
    x_labels = None
    indices = None
    
    bars = []
    bar_names = []
    curr_index = 0
    for label, group in label_groups.items():
        if x_labels is None:
            x_labels = [str(i) for i in group["input"]]
            indices = np.arange(len(x_labels))

        # I don't really grok the special type used by pandas, so just manually slap the data into an array.
        metrics = []
        for metric in group[args.metric]:
            metrics.append(metric)

        inputs = get_shifted_indices(indices, curr_index, num_bars, bar_width)
        bar = axes.bar(inputs, metrics, color=BarColours[curr_index], width=bar_width,align='edge')
        
        bar_names.append(label)
        bars.append(bar)
        curr_index += 1;

    axes.legend(bars, bar_names)
    fig.tight_layout()
    plt.xticks(indices, x_labels)

    if args.logy:
        plt.yscale("log")

    if outFileName:
        logging.info("Saving to %s" % outFileName)
        with contextlib.suppress(FileNotFoundError):
            os.remove(outFileName)
        fig.savefig(outFileName)
        
    if not args.quiet:
        plt.show()

def main():
    args = parse_args()
    data = read_data(args)
    categories = {}
    for category, categoryData in data.groupby("category"):
        label_groups = {}
        for label, group in categoryData.groupby("label"):
            label_groups[label] = group.set_index("input", drop=False)

        outputFile = None;
        if (args.output):
            outputFile = args.output + category + ".png"

        bar_plot_groups(category, label_groups, args, outputFile)

    if not args.quiet:
        plt.show()


if __name__ == "__main__":
    main()
