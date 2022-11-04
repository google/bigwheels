# Benchmarks
BigWheels includes a variety of benchmarks that test graphics fundamentals under the `benchmarks` folder. Benchmarks use special assets found in `assets/benchmarks`.

To build and run benchmarks, [build](building_and_running.md) BigWheels for your platform of choice. Then, you can run the benchmarks manually or use the provided helper script to run a group of benchmarks specified in a CSV file on a GGP instance.

All benchmarks support the `--stats-file path/to/stats.csv` option that controls where the results in CSV format are written to. Refer to a specific benchmark's code to determine which additional options they support.

## Running benchmarks manually on any platform
Once a benchmark is built, its binary will be in `bin/`. Simply run the binary along with any options you want.

Example:
```
bin/vk_texture_sample --stats-file results.csv --num-images 1 --force-mip-level 0 --filter-type linear
```

## Running a group of benchmarks on GGP
You can use the `tools/run-benchmarks.py` script to run a group of benchmarks on a GGP instance and automatically retrieve the results.

You can control which benchmarks are run, and with which options, by modifying `tools/benchmark_testcases.csv` or providing your own. The format of the testcase CSV file is:
```
short_name, descriptive_name, benchmark_binary_name, benchmark_binary_options
```

An example would be:
```
texture_load_1, Texture load (1 image), vk_texture_load, --num-images 1
```

Once you have a list of benchmarks you'd like to run, use the `run-benchmarks.py` script to dispatch their execution. You can control how many frames each benchmark runs for, and the directory where the results will be stored:
```
tools/run-benchmarks.py tools/benchmark_testcases.csv --instance=${USER}-1 --num_frames=20 --out=benchmark_results
```

## Analyzing benchmark results
Each benchmark is different, but all of the GPU benchmarks output a CSV file that contains per-frame performance results. The CSV format differs depending on each benchmark, but all contain at least the following information in the first three columns: frame number, GPU pipeline execution time in milliseconds, CPU frame time in milliseconds. You can refer to a specific benchmark's code to determine what other information is included.

You can use the `tools/compare-benchmark-results.py` script to compare a group of benchmarks across different platforms/settings.  This script accepts a list of directories, each containing benchmark results
from benchmark runs. The first results directory specified on the command line
is used as a baseline, against which all other results are compared against.

Example use:
```
tools/compare-benchmarks-results.py results_dir_1 results_dir_2 results_dir_3
```