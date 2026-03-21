"""
Format of 1 test case:
////////// TEST BEGIN
////////// 2026-03-21 18:14:34
thread=single,stderr=true,policy=drop: 3597884 logs/sec
thread=multi,stderr=true,policy=drop: 5281501 logs/sec
thread=single,stderr=false,policy=drop: 6764704 logs/sec
thread=multi,stderr=false,policy=drop: 3635111 logs/sec
thread=single,stderr=true,policy=block: 4757412 logs/sec
thread=multi,stderr=true,policy=block: 4653319 logs/sec
thread=single,stderr=false,policy=block: 7298179 logs/sec
thread=multi,stderr=false,policy=block: 6223019 logs/sec
////////// TEST END
"""

import re
import statistics
from collections import defaultdict

data = defaultdict(list)
with open("results.txt") as f:
    for line in f:
        m = re.match(r'thread=(\w+),stderr=(\w+),policy=(\w+): ([\d]+) logs/sec', line)
        if m:
            data[(m.group(1), m.group(2), m.group(3))].append(int(m.group(4)))
        else:
            # Auto-default to single thread if thread is not provided
            m = re.match(r'stderr=(\w+),policy=(\w+): ([\d]+) logs/sec', line)
            if m:
                data[('single', m.group(1), m.group(2))].append(int(m.group(3)))

def cell(values):
    if not values:
        return ["N/A"] * 5
    stdev = f"{statistics.stdev(values):,.2f} logs/sec" if len(values) > 1 else "N/A"
    return [
        f"{min(values):,.2f} logs/sec",
        f"{max(values):,.2f} logs/sec",
        f"{statistics.mean(values):,.2f} logs/sec",
        f"{statistics.median(values):,.2f} logs/sec",
        f"{stdev}",
    ]

threads = ["single", "multi"]
policies = ["drop", "block"]
labels = ["Minimum", "Maximum", "Average", "Median", "Std Dev"]

col_w = 25
def row(a, b, c):
    return f"| {a:<{col_w-2}} | {b:<{col_w-2}} | {c:<{col_w-2}} |"

def capitalize(s):
    return s[0].upper() + s[1:len(s)].lower()

for active_thread in threads:
    sep = "+" + ("-" * (col_w)) + "+" + ("-" * col_w) + "+" + ("-" * col_w) + "+"
    print(sep)
    print(row(f"{capitalize(active_thread)} Threaded", "WITH stderr sink", "WITHOUT stderr sink"))
    print(sep)
    for policy in policies:
        yes_lines = cell(data.get((active_thread, "true", policy), []))
        no_lines  = cell(data.get((active_thread, "false", policy), []))
        n_yes = len(data.get((active_thread, "true", policy), []))
        n_no  = len(data.get((active_thread, "false", policy), []))
        print(row(f"{capitalize(policy)} Policy", f"({n_yes} runs)", f"({n_no} runs)"))
        for y, n, label in zip(yes_lines, no_lines, labels):
            print(row(label, y, n))
        print(sep)
    print("")
