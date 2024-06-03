#!/usr/bin/env python3
import json

def get_mpki(folder):

    with open(str(folder.joinpath('result.json')), 'r') as jsonfile:
        data = json.load(jsonfile)

        num_traces = 0
        value = 0

        for trace in data.values():
            num_traces += 1
            value += float(trace['MISPRED_PER_1K_INST'])

        mpki = value / num_traces

        return num_traces, mpki
