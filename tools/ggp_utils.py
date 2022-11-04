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

"""Utilities to interact with a GGP instance using the ggp utility."""

import logging
import os
import subprocess


def SyncFilesToInstance(ggp_bin, instance, sources, dst):
  """Transfers the given file(s) to the given instance.

  Args:
    ggp_bin: Path to the ggp executable.
    instance: The instance name or ID. Can be none, if only one instance is
      reserved.
    sources: A list of local paths to synchronize.
    dst: The path of the destination directory on the instance.

  Returns:
    A return code value. 0 means success.
  """
  cmd = [ggp_bin, 'ssh', 'sync', '-r', '--progress', '-R']
  if instance is not None:
    cmd.extend(['--instance', instance])
  cmd.extend(sources)
  cmd.append(dst)
  logging.info('$ %s', ' '.join(cmd))
  return subprocess.call(cmd)


def GetFilesFromInstance(ggp_bin, instance, sources, dst):
  """Transfers the given file(s) from the given instance to the local machine.

  Args:
    ggp_bin: Path to the ggp executable.
    instance: The instance name or ID. Can be none, if only one instance is
      reserved.
    sources: A list of paths on the instance to retrieve.
    dst: The path of the destination directory on the local machine.

  Returns:
    A return code value. 0 means success.
  """
  cmd = [ggp_bin, 'ssh', 'get', '-r']
  if instance is not None:
    cmd.extend(['--instance', instance])
  cmd.extend(sources)
  cmd.append(dst)
  logging.info('$ %s', ' '.join(cmd))
  return subprocess.call(cmd)


def RunOnInstance(ggp_bin, instance, app_path, binary, binary_args, ggp_vars):
  """Runs the given binary on the instance.

  Args:
    ggp_bin: Path to the ggp executable.
    instance: The instance name or ID. Can be none, if only one instance is
      reserved.
    app_path: Path relative to /mnt/developer where the binary can be found.
    binary: The binary to execute.
    binary_args: The command-line arguments to pass to the binary.
    ggp_vars: The --vars string to pass to the ggp binary.

  Returns:
    A return code value. 0 means success.
  """

  binary_cmd = '%s/%s %s' % (app_path, binary, binary_args)

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


def RunOnInstanceHeadless(ggp_bin, instance, app_path, binary, binary_args,
                          env_vars):
  """Runs the given binary on the instance, in headless mode (no endpoint).

  This is achieved by SSH-ing into into the instance and running the binary
  directly.

  Args:
    ggp_bin: Path to the ggp executable.
    instance: The instance name or ID. Can be none, if only one instance is
      reserved.
    app_path: Path relative to /mnt/developer where the binary can be found.
    binary: The binary to execute.
    binary_args: The command-line arguments to pass to the binary.
    env_vars: Environment variables to set when running the binary.

  Returns:
    A return code value. 0 means success.
  """

  binary_cmd = '%s/%s %s' % (app_path, os.path.basename(binary), binary_args)
  full_ssh_command = ('cd /mnt/developer; %s %s') % (env_vars, binary_cmd)

  cmd = [
      ggp_bin,
      'ssh',
      'shell',
  ]
  if instance is not None:
    cmd.extend(['--instance', instance])
  cmd.extend(['--', full_ssh_command])
  logging.info('$ %s', ' '.join(cmd))
  return subprocess.call(cmd)


def TerminateProcessOnInstance(ggp_bin, instance, process_name):
  """Forcefully terminate the given process on the instance.

  Args:
    ggp_bin: Path to the ggp executable.
    instance: The instance name or ID. Can be none, if only one instance is
      reserved.
    process_name: The name of the process to kill.

  Returns:
    A return code value. 0 means success.
  """

  ssh_command = 'killall %s || echo "Already killed."' % process_name

  cmd = [
      ggp_bin,
      'ssh',
      'shell',
  ]
  if instance is not None:
    cmd.extend(['--instance', instance])
  cmd.extend(['--', ssh_command])
  logging.info('$ %s', ' '.join(cmd))
  return subprocess.call(cmd)
