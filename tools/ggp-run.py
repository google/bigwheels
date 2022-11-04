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

"""GGP driver for BigWheels binaries.

This driver uploads a BigWheels project binary and its assets to a Stadia
instance.  Once uploaded, it executes it using 'ggp run'.

Example use,

$ tools/ggp-run.py bin/vk_fishtornado
"""

import argparse
import logging
import os
import subprocess
import sys
import ggp_utils

# Default location for the 'ggp' binary.
_GGP_BIN = 'ggp'


def _RunOnInstance(ggp_bin, instance, app_path, binary, binary_args, ggp_vars):
  """Runs the given binary on the instance.

  Args:
    ggp_bin: Path to the ggp executable.
    instance: The instance name or ID. None, if only one instance is reserved.
    app_path: Path relative to /mnt/developer where the binary can be found.
    binary: The binary to execute.
    binary_args: The command-line arguments to pass to the binary.
    ggp_vars: The --vars string to pass to the ggp binary.

  Returns:
    A return code value. 0 means success.
  """

  binary_cmd = '%s/%s %s' % (app_path, os.path.basename(binary), binary_args)

  cmd = [
      ggp_bin, 'run', '--no-launch-browser',
      '--application=TestApp', '--cmd', binary_cmd
  ]
  if instance is not None:
    cmd.extend(['--instance', instance])
  if ggp_vars:
    cmd.extend(['--vars', ggp_vars])
  logging.info('$ %s', ' '.join(cmd))
  return subprocess.call(cmd)


def main():
  logging.basicConfig(
      format='%(asctime)s %(module)s: %(message)s', level=logging.INFO)

  parser = argparse.ArgumentParser(
      description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
  parser.add_argument(
      '--instance',
      type=str,
      default=None,
      help='The instance name or ID to set up. This instance must be already '
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
      'the application (default: %(default)s).',
  )
  parser.add_argument(
      'binary',
      help='Binary to execute on the instance.',
  )
  parser.add_argument(
      '--binary_args',
      default='',
      help='The command-line arguments to pass to the binary.',
  )
  parser.add_argument(
      '--vars',
      default='',
      help='The --vars string to pass to the ggp binary as part of the `ggp run` command.',
  )
  args = parser.parse_args()

  sources = ['assets', args.binary]
  rc = ggp_utils.SyncFilesToInstance(args.ggp_bin, args.instance, sources,
                                     '/mnt/developer/%s/' % args.app_path)
  if rc != 0:
    return rc

  return ggp_utils.RunOnInstance(args.ggp_bin, args.instance, args.app_path,
                                 args.binary, args.binary_args, args.vars)


if __name__ == '__main__':
  sys.exit(main())
