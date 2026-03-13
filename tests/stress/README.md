# Stress Test

- This scripts are testing the logger with 1 million logs
- Running `make` will compile the stress.c and logger.h and spits out "app"
- Running `./app` will print out configurations, dropped logs, elapsed time and time per log (in nanoseconds)
- App accepts parameters: first parameter is the log policy which can be:
  -> `drop`, `block`, `priority_based`
  and second parameter is "will logger print to stderr?"
  -> `true` or `false`
- `benchmark.sh` will run the app with all possible parameters and append the results into results.txt file
- `test.sh` will run `benchmark.sh` for specified times which can be specified with number parameter like `test.sh 10`.
  and it will clear the results.txt file and puts all the results to the file.
- `analyze.py` will do basic data analysis from results.txt file and generates 2x2 table where rows are policies and columns are stderr specifier. (Does avg, min, max, median, stdev)

# Results on my machine

- With 16 GiB RAM, Intel Core i5-8500 6 Cores, 4.0 GHz, x86_64
- Compiled with GCC, on GNU/Linux:

```
+--------------------+--------------------+--------------------+
| Per Log (NS)       | WITH stderr sink   | WITHOUT stderr sink|
+--------------------+--------------------+--------------------+
| Drop Policy        | (1000 runs)        | (1000 runs)        |
| Minimum            | 23.27 ns           | 37.55 ns           |
| Maximum            | 30.90 ns           | 65.29 ns           |
| Average            | 24.06 ns           | 45.67 ns           |
| Median             | 23.97 ns           | 45.23 ns           |
| Std Dev            | 0.68 ns            | 2.64 ns            |
+--------------------+--------------------+--------------------+
| Block Policy       | (1000 runs)        | (1000 runs)        |
| Minimum            | 798.36 ns          | 139.01 ns          |
| Maximum            | 865.24 ns          | 152.71 ns          |
| Average            | 810.36 ns          | 145.07 ns          |
| Median             | 809.72 ns          | 144.94 ns          |
| Std Dev            | 5.66 ns            | 2.33 ns            |
+--------------------+--------------------+--------------------+
```

- NOTE: Block policy does not drop any log, producer will block the main thread
