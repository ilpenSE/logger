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

- With 16 GiB RAM, Intel Core i5-8500 6 Cores, 4.0 GHz and 100 runs in a row:

+--------------------+--------------------+--------------------+
| Per Log (NS)       | WITH stderr sink   | WITHOUT stderr sink |
+--------------------+--------------------+--------------------+
| Drop Policy       | (100 runs)         | (100 runs)         |
| Minimum            | 35.34 ns           | 47.00 ns           |
| Maximum            | 83.85 ns           | 56.88 ns           |
| Average            | 37.24 ns           | 51.96 ns           |
| Median             | 36.09 ns           | 51.88 ns           |
| Std Dev            | 4.91 ns            | 2.07 ns            |
+--------------------+--------------------+--------------------+
| Block Policy       | (100 runs)         | (100 runs)         |
| Minimum            | 524.55 ns          | 149.35 ns          |
| Maximum            | 593.15 ns          | 204.41 ns          |
| Average            | 539.66 ns          | 155.86 ns          |
| Median             | 530.87 ns          | 153.83 ns          |
| Std Dev            | 17.17 ns           | 7.25 ns            |
+--------------------+--------------------+--------------------+

- NOTE: Block policy does not drop any log, producer will block the main thread
