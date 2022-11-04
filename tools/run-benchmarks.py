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

"""Script to run all the benchmarks on a GGP instance and retrieve results.

This script uploads the BigWheels benchmark binaries and their assets to a GGP
instance.  Once uploaded, it executes them remotely by running them in headless
mode, and it retrieves the CSV statistics file that the benchmarks produce.

Example use:
$ tools/run-benchmarks.py tools/benchmark_testcases.csv --out benchmark_results
"""

import argparse
import csv
import logging
import os
import sys
import ggp_utils

# Default location for the 'ggp' binary.
_GGP_BIN = 'ggp'


class BenchmarkTestCase:
  """A single benchmark test case."""

  def __init__(self, short_name, descriptive_name, binary, binary_args):
    self.short_name = short_name
    self.descriptive_name = descriptive_name
    self.binary = binary
    self.binary_args = binary_args

  def GetDescriptiveName(self):
    return self.descriptive_name

  def GetShortName(self):
    return self.short_name

  def GetCsvOutputFilename(self):
    return self.short_name + '.csv'

  def GetBinaryName(self):
    return self.binary

  def GetBinaryArgs(self):
    return self.binary_args

  def __str__(self):
    return '%s (%s %s)' % (self.descriptive_name, self.binary, self.binary_args)

  def __repr__(self):
    return self.__str__()


def CollectBenchmarkTestCases(testcases_file):
  """Reads a testcase file in CSV format and collect test cases."""
  test_cases = []
  with open(testcases_file) as f:
    r = csv.reader(f, delimiter=',')
    for row in r:
      if not row:
        continue
      if len(row) != 4:
        logging.warning('Row %s is invalid, it will be skipped', row)
        continue

      test_cases.append(
          BenchmarkTestCase(row[0].strip(), row[1].strip(), row[2].strip(),
                            row[3].strip()))
  return test_cases


def ProcessArgs():
  """Process command-line flags."""
  parser = argparse.ArgumentParser(
      description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
  parser.add_argument(
      '--instance',
      type=str,
      default=None,
      help='The instance name or ID to use. This instance must be already '
      'reserved.  If you only have a single instance reserved, this can be '
      'empty (default: %(default)s).',
  )
  parser.add_argument(
      '--ggp_bin',
      default=_GGP_BIN,
      help='Path to the "ggp" binary to use (default: %(default)s).',
  )
  parser.add_argument(
      '--app_path',
      default='bw',
      help='Path relative to /mnt/developer where to store and execute '
      'the benchmarks and benchmark outputs (default: %(default)s).',
  )
  parser.add_argument(
      '--vars',
      default='',
      help='Environment vars to pass to the benchmark binaries.',
  )
  parser.add_argument(
      '--num_frames',
      default=20,
      help='Number of frames to run each benchmark for (default: %(default)s).',
  )
  parser.add_argument(
      '--out',
      default='benchmark_results',
      help='Path to the local directory where benchmark results will be saved. '
      'The directory will be created if it does not exist (default: %(default)s).',
  )
  parser.add_argument(
      'testcases_file',
      help='Path to a CSV file containing benchmark testcases to run. The CSV format '
      'is `short_name, descriptive_name, benchmark_binary_name, binary_flags`, '
      'without a header.',
  )
  args = parser.parse_args()
  return args


def RunBenchmarkTestCase(test_case, ggp_bin, instance, app_path, env_vars,
                         num_frames, out_dir):
  """Run a benchmark test case and retrieve results."""
  csv_file_path = os.path.join('/mnt/developer', app_path,
                               test_case.GetCsvOutputFilename())
  stats_file_arg = '--stats-file %s' % csv_file_path
  num_frames_arg = '--frame-count %s' % num_frames
  binary_args = ' '.join(
      [test_case.GetBinaryArgs(), stats_file_arg, num_frames_arg])

  logging.info('Running benchmark "%s"', test_case.GetDescriptiveName())
  rc = ggp_utils.RunOnInstanceHeadless(ggp_bin, instance, app_path,
                                       test_case.GetBinaryName(), binary_args,
                                       env_vars)

  if rc != 0:
    return rc

  rc = ggp_utils.TerminateProcessOnInstance(ggp_bin, instance,
                                            test_case.GetBinaryName())

  if rc != 0:
    return rc

  return ggp_utils.GetFilesFromInstance(ggp_bin, instance, [csv_file_path],
                                        out_dir)


def main():
  logging.basicConfig(
      format='%(asctime)s %(module)s: %(message)s', level=logging.INFO)

  args = ProcessArgs()

  test_cases = CollectBenchmarkTestCases(args.testcases_file)
  if not test_cases:
    logging.error('No test cases specified.')
    return -1

  # Upload assets and benchmark binaries.
  sources = ['assets']
  sources.extend(set(['bin/' + tc.GetBinaryName() for tc in test_cases]))
  rc = ggp_utils.SyncFilesToInstance(args.ggp_bin, args.instance, sources,
                                     '/mnt/developer/%s/' % args.app_path)
  if rc != 0:
    return rc

  os.makedirs(args.out, exist_ok=True)

  # Execute each benchmark test case and retrieve CSV result output.
  logging.info('Executing %s benchmarks, storing results in %s',
               len(test_cases), args.out)
  for tc in test_cases:
    rc = RunBenchmarkTestCase(tc, args.ggp_bin, args.instance, args.app_path,
                              args.vars, args.num_frames, args.out)
    if rc != 0:
      logging.error(
          'Benchmark "%s" failed to execute or results are unavailable',
          tc.GetDescriptiveName)

  return 0


if __name__ == '__main__':
  sys.exit(main())
