# Stress Test

- This scripts are testing the logger with 1 million logs (you can change it via LOGS_COUNT macro)
- Running `make` will compile the stress.c and logger.h and spits out "app"
- Running `./app` will print out configurations, dropped logs, elapsed time and time per log (in nanoseconds)
- App accepts parameters: first parameter is the log policy which can be:
  -> `drop`, `block`, `priority_based`
  and second parameter is "will logger print to stderr?"
  -> `true` or `false`
- `benchmark.sh` will run the app with all possible parameters and append the results into results.txt file
- `testn.sh` will run `benchmark.sh` for specified times which can be specified with number parameter
like `testn.sh 10`. and it will clear the results.txt file and puts all the results to the file.
- `analyze.py` will do basic data analysis from results.txt file and generates 2x2 table where rows are policies and columns are stderr specifier. (Does avg, min, max, median, stdev)

# Results on my machine

- With 16 GiB RAM, Intel Core i5-8500 6 Cores, 4.0 GHz, x86_64
- Compiled with GCC, on GNU/Linux:

```
+-------------------------+-------------------------+-------------------------+
| Single Threaded         | WITH stderr sink        | WITHOUT stderr sink     |
+-------------------------+-------------------------+-------------------------+
| Drop Policy             | (1000 runs)             | (1000 runs)             |
| Minimum                 | 958,066.00 logs/sec     | 2,389,868.00 logs/sec   |
| Maximum                 | 4,796,818.00 logs/sec   | 6,865,822.00 logs/sec   |
| Average                 | 4,569,206.02 logs/sec   | 6,473,898.50 logs/sec   |
| Median                  | 4,596,644.50 logs/sec   | 6,521,836.00 logs/sec   |
| Std Dev                 | 222,177.09 logs/sec     | 297,800.32 logs/sec     |
+-------------------------+-------------------------+-------------------------+
| Block Policy            | (1000 runs)             | (1000 runs)             |
| Minimum                 | 4,101,427.00 logs/sec   | 5,225,708.00 logs/sec   |
| Maximum                 | 4,748,450.00 logs/sec   | 6,951,432.00 logs/sec   |
| Average                 | 4,585,471.56 logs/sec   | 6,641,610.29 logs/sec   |
| Median                  | 4,593,321.50 logs/sec   | 6,651,180.00 logs/sec   |
| Std Dev                 | 78,230.23 logs/sec      | 144,623.82 logs/sec     |
+-------------------------+-------------------------+-------------------------+
```

- NOTE: Block policy does not drop any log, producer will block the main thread
