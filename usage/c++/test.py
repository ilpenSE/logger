import sys
import re
from collections import defaultdict

values = list()

pattern = re.compile(r"thread (\d+), i = (\d+)")
per_thread = defaultdict(set)

bad_lines = 0
line_count = 0
other_lines = 0

logf = "logs/2026.01.27-16.48.17.105.log"
with open(logf, "r", errors="replace") as f:
  for i, line in enumerate(f, 1):
    line_count += 1
    m = pattern.search(line)
    if m:
      tid = int(m.group(1))
      i = int(m.group(2))
      per_thread[tid].add(i)
    if not line.endswith("\n"):
      bad_lines += 1
    if not line.startswith("2026.01.27"):
      other_lines += 1
    
missing_total = 0
for tid, values in per_thread.items():
  missing = set(range(1000)) - values
  if missing:
    print(f"Thread {tid} missing {len(missing)} entries")
    missing_total += len(missing)
        
with open("time.txt") as f:
  values = [float(line.strip()) for line in f if line.strip()]

sumv  = sum(values)
count = len(values)
minv  = min(values)
maxv  = max(values)
avg   = sumv / count

print(f"Total Lines : {line_count}")
print(f"Bad Lines (does not end with newl) : {bad_lines}")
print(f"Lines that contain something else : {other_lines}")
print("Total threads seen :", len(per_thread))
print("Total missing logs :", missing_total)

print(f"Count   : {count}")
print(f"Sum     : {sumv} s")
print(f"Average : {avg} s")
print(f"Maximum : {maxv} s")
print(f"Minimum : {minv} s")
