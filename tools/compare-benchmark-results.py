#!/usr/bin/env python3

# Copyright 2022 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Compare a set of benchmark results, using the first as the baseline.

This script accepts a list of directories, each containing benchmark results
from benchmark runs. The first results directory specified on the command line
is used as a baseline, against which all other results are compared against.

The following is a valid directory structure:
-- results_dir_1
-- -- texture_load_1.csv
-- -- texture_load_4.csv
-- results_dir_2
-- -- texture_load_1.csv
-- -- texture_load_4.csv
-- results_dir_3
-- -- texture_load_1.csv
-- -- texture_load_4.csv

Example use:
$ tools/compare-benchmarks-results.py results_dir_1 results_dir_2 results_dir_3
"""

import argparse
import csv
import dataclasses
import logging
import os
import statistics
import sys

# Metric names (from benchmark output format).
_CSV_BENCHMARK_METRICS = ['Pipeline GPU time (ms)', 'Frame CPU time (ms)']


@dataclasses.dataclass
class FrameDatapoint:
  num_frame: int
  metrics: list[float]


@dataclasses.dataclass
class AggregatedTestMetric:
  name: str
  min: float
  avg: float
  median: float
  max: float


@dataclasses.dataclass
class TestResults:
  """A collection of per-frame datapoints."""
  frame_datapoints: list[FrameDatapoint]

  def GetMetric(self, metric_index):
    data = [frame.metrics[metric_index] for frame in self.frame_datapoints]
    return AggregatedTestMetric(_CSV_BENCHMARK_METRICS[metric_index], min(data),
                                statistics.mean(data), statistics.median(data),
                                max(data))

  def ContainsDatapoints(self):
    return self.frame_datapoints


def CollectBenchmarkTestResults(results_dir):
  """Collect benchmark results file names by benchmark test case name.

  Args:
    results_dir: The directory where CSV benchmark test results are present.

  Returns:
    A dictionary that maps test case names to the filename containing results.
  """
  test_cases = dict()
  for _, _, filenames in os.walk(results_dir):
    for filename in sorted(filenames):
      if filename.endswith('.csv'):
        test_cases[filename[:-4]] = os.path.join(results_dir, filename)
  return test_cases


def ReadTestResults(result_filename, num_frames_to_ignore):
  """Read test results given a path to a CSV benchmark file.

  Args:
    result_filename: The path to the CSV output from a benchmark run.
    num_frames_to_ignore: The number N of frames to ignore. The datapoints of
      the first N frames will be ignored when reading results.

  Returns:
    The parsed test results.
  """
  frame_datapoints = []
  with open(result_filename) as f:
    r = csv.reader(f, delimiter=',')
    for row in r:
      if len(row) < 3:
        logging.error('Invalid result CSV format for file %s', result_filename)
        return TestResults()
      frame_num = int(row[0])
      if frame_num < 0:
        logging.error('Invalid frame number %s found in CSV file %s', frame_num,
                      result_filename)
        return TestResults()
      if frame_num <= num_frames_to_ignore:
        continue
      frame_datapoints.append(
          FrameDatapoint(frame_num,
                         [float(row[1]), float(row[2])]))

  return TestResults(frame_datapoints)


def GetPercentageDiff(first, second):
  """Get the percentage difference of `second` over `first`."""
  if first == 0:
    first = 1e-10
  return ((second - first) * 100.0) / first


def PrintMetricResultsBaseline(results_name, metric):
  """Print the values of a baseline metric."""
  print('--- {}, Min/Mean/Median/Max'.format(metric.name))
  print('\t[{}] {:.2f}, {:.2f}, {:.2f}, {:.2f}'.format(results_name, metric.min,
                                                       metric.avg,
                                                       metric.median,
                                                       metric.max))


def PrintMetricResultsComparison(name_baseline, name_other, metric_baseline,
                                 metric_other):
  """Print the values of a metric against a baseline.

  Args:
    name_baseline: The name of the baseline results.
    name_other: The name of the results being compared against the baseline.
    metric_baseline: The baseline metric values.
    metric_other: The metric values being compared against the baseline.
  """
  assert metric_baseline.name == metric_other.name

  percentage_diff_min = GetPercentageDiff(metric_baseline.min, metric_other.min)
  percentage_diff_avg = GetPercentageDiff(metric_baseline.avg, metric_other.avg)
  percentage_diff_median = GetPercentageDiff(metric_baseline.median,
                                             metric_other.median)
  percentage_diff_max = GetPercentageDiff(metric_baseline.max, metric_other.max)

  print(
      '\t[{}] {:.2f}, {:.2f}, {:.2f}, {:.2f} ({:+5.2f}%, {:+5.2f}%, {:+5.2f}%, {:+5.2f}% vs {})'
      .format(name_other, metric_other.min, metric_other.avg,
              metric_other.median, metric_other.max, percentage_diff_min,
              percentage_diff_avg, percentage_diff_median, percentage_diff_max,
              name_baseline))


def CompareTestResults(results, names, num_frames_to_ignore):
  """Compare a set of benchmark results, using the first as the baseline."""
  base_results = results[0]
  base_name = names[0]

  for test_name in base_results:
    print('Benchmark %s:' % test_name)

    # For each metric measured, print baseline and comparisons.
    base_data = ReadTestResults(base_results[test_name], num_frames_to_ignore)
    for m in range(0, len(_CSV_BENCHMARK_METRICS)):
      PrintMetricResultsBaseline(base_name, base_data.GetMetric(m))

      # Output the metric values from all other benchmarks results,
      # showing a percentage comparison against the baseline.
      for i in range(1, len(results)):
        other_results = results[i]
        other_name = names[i]
        if test_name not in other_results:
          print('\t[%s] No data' % other_name)
          continue

        other_data = ReadTestResults(other_results[test_name],
                                     num_frames_to_ignore)
        PrintMetricResultsComparison(base_name, other_name,
                                     base_data.GetMetric(m),
                                     other_data.GetMetric(m))

    print('')

  return 0


def ProcessArgs():
  """Process command-line flags."""
  parser = argparse.ArgumentParser(
      description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
  parser.add_argument(
      'results_dirs',
      nargs='+',
      default=[],
      help='A list of directories where the benchmark results are stored. '
      'The first benchmark results will be compared to each of the others',
  )
  parser.add_argument(
      '--ignore_first_N_frames',
      type=int,
      default=1,
      help='The number of frames to ignore. Frames 1..N will be ignored when '
      'reading test results. If set to zero, all frames will be processed. '
      'Default is set to 1 as the first frame usually contains setup times.',
  )

  args = parser.parse_args()
  return args


def main():
  logging.basicConfig(
      format='%(asctime)s %(module)s: %(message)s', level=logging.INFO)

  args = ProcessArgs()

  if len(args.results_dirs) < 2:
    logging.error('Not enough benchmark result directories specified, at least '
                  'two required to perform comparisons')
    return -1

  dirs = [os.path.normpath(d) for d in args.results_dirs]
  for d in dirs:
    if not os.path.exists(d) or not os.path.isdir(d):
      logging.error('Path %s is not a valid directory', d)
      return -1

  results = [CollectBenchmarkTestResults(d) for d in dirs]
  names = [os.path.basename(d) for d in dirs]
  return CompareTestResults(results, names, args.ignore_first_N_frames)


if __name__ == '__main__':
  sys.exit(main())
