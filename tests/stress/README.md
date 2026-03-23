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
| Minimum                 | 8,762,583.00 logs/sec   | 15,457,015.00 logs/sec  |
| Maximum                 | 31,733,203.00 logs/sec  | 28,715,350.00 logs/sec  |
| Average                 | 23,296,825.07 logs/sec  | 21,169,493.64 logs/sec  |
| Median                  | 23,367,381.00 logs/sec  | 21,445,465.50 logs/sec  |
| Std Dev                 | 1,050,903.81 logs/sec   | 980,765.08 logs/sec     |
+-------------------------+-------------------------+-------------------------+
| Block Policy            | (1000 runs)             | (1000 runs)             |
| Minimum                 | 4,283,080.00 logs/sec   | 5,978,928.00 logs/sec   |
| Maximum                 | 9,066,669.00 logs/sec   | 21,739,206.00 logs/sec  |
| Average                 | 8,683,404.37 logs/sec   | 20,262,225.76 logs/sec  |
| Median                  | 8,727,793.50 logs/sec   | 20,824,819.50 logs/sec  |
| Std Dev                 | 379,101.58 logs/sec     | 1,696,879.13 logs/sec   |
+-------------------------+-------------------------+-------------------------+

+-------------------------+-------------------------+-------------------------+
| Multi Threaded          | WITH stderr sink        | WITHOUT stderr sink     |
+-------------------------+-------------------------+-------------------------+
| Drop Policy             | (1000 runs)             | (1000 runs)             |
| Minimum                 | 6,523,412.00 logs/sec   | 3,639,914.00 logs/sec   |
| Maximum                 | 125,471,301.00 logs/sec | 124,757,035.00 logs/sec |
| Average                 | 47,807,668.97 logs/sec  | 39,445,953.86 logs/sec  |
| Median                  | 43,634,190.00 logs/sec  | 36,502,051.50 logs/sec  |
| Std Dev                 | 15,950,487.84 logs/sec  | 16,092,290.69 logs/sec  |
+-------------------------+-------------------------+-------------------------+
| Block Policy            | (1000 runs)             | (1000 runs)             |
| Minimum                 | 1,547,147.00 logs/sec   | 1,747,256.00 logs/sec   |
| Maximum                 | 8,866,158.00 logs/sec   | 12,858,595.00 logs/sec  |
| Average                 | 7,004,907.44 logs/sec   | 9,801,498.71 logs/sec   |
| Median                  | 6,799,297.50 logs/sec   | 9,869,682.00 logs/sec   |
| Std Dev                 | 871,647.96 logs/sec     | 1,060,951.82 logs/sec   |
+-------------------------+-------------------------+-------------------------+
```

- NOTE: Block policy does not drop any log, producer will block the main thread
