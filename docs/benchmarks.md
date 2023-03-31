# Benchmarks
BigWheels includes a variety of benchmarks that test graphics fundamentals under the `benchmarks` folder. Benchmarks use special assets found in `assets/benchmarks`.

To build and run benchmarks, [build](building_and_running.md) BigWheels for your platform of choice. Then, you can run the benchmarks manually.

All benchmarks support the `--stats-file path/to/stats.csv` option that controls where the results in CSV format are written to. Refer to a specific benchmark's code to determine which additional options they support.

## Running benchmarks manually on any platform
Once a benchmark is built, its binary will be in `bin/`. Simply run the binary along with any options you want.

Example:
```
bin/vk_texture_sample --stats-file results.csv --num-images 1 --force-mip-level 0 --filter-type linear
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