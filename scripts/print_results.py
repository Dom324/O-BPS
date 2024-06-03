#!/usr/bin/env python3
import json
from collections import defaultdict
import click
from tabulate import tabulate
import textwrap
import os
import sys
from . import myconstants as mc

@click.command(context_settings=mc.CLIC_CONTEXT_SETTINGS)
@click.argument('dirs', nargs=-1,   type=mc.CLICK_R_DIR, required=True)
@click.option("-m", "--metric",     default="MISPRED_PER_1K_INST", help="Metric to show", show_default=True)
@click.option("-s", "--sort_stats", default=False,                 help="Sort stats according to the first directory")
def print_results(dirs, metric, sort_stats):
    """Print choosen metric from given result directories"""

    result_dict = defaultdict(dict)

    for directory in dirs:
        with open(directory + "/result.json", 'r') as f:
            data = json.load(f)

            for trace, stats in data.items():
                result_dict[trace][directory] = stats["MISPRED_PER_1K_INST"]

    tabulate_list = []
    bad_comparison = False
    mpki_sum = [0] * len(dirs)
    num_traces = [0] * len(dirs)

    for trace in result_dict:
        tabulate_list.append([trace])
        for count, directory in enumerate(dirs):
            try:
                num = float(result_dict[trace][directory])
                mpki_sum[count] += num
                num_traces[count] += 1
                tabulate_list[-1].append(num)
            except:
                bad_comparison = True
                tabulate_list[-1].append("")

# TODO
#    dummy = ["-------"]
#    for count, value in enumerate(dirs):
#        dummy.append("-------")
#    tabulate_list.append(dummy)

    footer = ["Average: "]
    for count, value in enumerate(dirs):
        footer.append(mpki_sum[count] / num_traces[count])
    tabulate_list.append(footer)

    footer2 = ["Number of traces: "]
    for count, value in enumerate(dirs):
        footer2.append(num_traces[count])
    tabulate_list.append(footer2)

    dirs_wrapped = [textwrap.fill(os.path.relpath(d), width=30) for d in dirs]
    print(tabulate(tabulate_list, floatfmt=".3f", headers=["Trace name"] + dirs_wrapped))

    if bad_comparison:
        print("\nWARNING: Bad comparison, some results are missing")

