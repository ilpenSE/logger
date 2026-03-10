"""
Format of 1 test case:
////////// TEST BEGIN
////////// 2026-03-10 19:03:08
stderr=true,policy=drop: 43.42 ns
stderr=false,policy=drop: 50.27 ns
stderr=true,policy=block: 546.37 ns
stderr=false,policy=block: 147.28 ns
////////// TEST END
"""

import re
import statistics
from collections import defaultdict

data = defaultdict(list)
with open("results.txt") as f:
    for line in f:
        m = re.match(r'stderr=(\w+),policy=(\w+): ([\d.]+) ns', line)
        if m:
            data[(m.group(2), m.group(1))].append(float(m.group(3)))

def cell(values):
    if not values:
        return ["N/A"] * 5
    stdev = f"{statistics.stdev(values):.2f} ns" if len(values) > 1 else "N/A"
    return [
        f"{min(values):.2f} ns",
        f"{max(values):.2f} ns",
        f"{statistics.mean(values):.2f} ns",
        f"{statistics.median(values):.2f} ns",
        f"{stdev}",
    ]

policies = ["drop", "block"]
labels = ["Minimum", "Maximum", "Average", "Median", "Std Dev"]

col_w = 20
sep = "+" + ("-" * (col_w)) + "+" + ("-" * col_w) + "+" + ("-" * col_w) + "+"

def row(a, b, c):
    return f"| {a:<{col_w-2}} | {b:<{col_w-2}} | {c:<{col_w-2}} |"

print(sep)
print(row("Per Log (NS)", "WITH stderr sink", "WITHOUT stderr sink"))
print(sep)

def capitalize(s):
    return s[0].upper() + s[1:len(s)].lower()

for policy in policies:
    yes_lines = cell(data.get((policy, "true"), []))
    no_lines  = cell(data.get((policy, "false"), []))
    n_yes = len(data.get((policy, "true"), []))
    n_no  = len(data.get((policy, "false"), []))

    print(row(f"{capitalize(policy)} Policy", f"({n_yes} runs)", f"({n_no} runs)"))
    for y, n, label in zip(yes_lines, no_lines, labels):
        print(row(label, y, n))
    print(sep)
