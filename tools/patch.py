#!/usr/bin/python
#
# Copyright 2016 LINE Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Patch all.gyp file to include stellite.gyp"""

import argparse
import logging
import os
import platform
import subprocess
import sys

LOCAL_PATH = os.path.dirname(os.path.abspath(__file__))
CHROMIUM_SRC = os.path.dirname(os.path.dirname(LOCAL_PATH))
PATCH_PATH = os.path.join(CHROMIUM_SRC, 'stellite', 'patches')

WINDOWS = 'windows'


def patch_git_apply(working_dir, patch_file, target_dir=None):
  """Patch a repository using git apply"""
  if not os.path.exists(patch_file):
    print '{} patch file is not exist'.format(patch_file)
    return

  command = ['git', 'apply', '--ignore-space-change']
  if target_dir != None:
    command.extend(['--directory', target_dir])
  command.append(patch_file)

  print 'command: ' + ' '.join(command)

  #ignore exception
  try:
    shell = platform.system().lower() == WINDOWS
    subprocess.Popen(command, cwd=working_dir, shell=shell).wait()
  except Exception:
    pass


def patch_build_script():
  """Patch build/all.gyp"""
  working_dir = CHROMIUM_SRC
  target_dir = 'build'
  gyp_patch_file = os.path.join(PATCH_PATH, 'build_gyp.patch')

  patch_git_apply(working_dir, gyp_patch_file, target_dir)


def patch_boringssl():
  """Patch boringssl to include bio, and something ..."""
  working_dir = CHROMIUM_SRC
  target_dir = os.path.join('third_party', 'boringssl')
  boringssl_patch_file = os.path.join(PATCH_PATH, 'boringssl_gypi.patch')

  patch_git_apply(working_dir, boringssl_patch_file)


def main(argv):
  """Main entry point"""
  argument_parser = argparse.ArgumentParser()
  argument_parser.add_argument('--no_boringssl', action='store_true')
  args = argument_parser.parse_args(argv)

  patch_build_script()

  if not args.no_boringssl:
    patch_boringssl()


if __name__ == '__main__':
  main(sys.argv[1:])
