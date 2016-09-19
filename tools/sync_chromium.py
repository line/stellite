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

from subprocess import CalledProcessError

import logging
import os
import platform
import subprocess
import sys
import traceback

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
STELLITE_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, os.pardir))
CHROME_SRC = os.path.abspath(os.path.join(STELLITE_DIR, os.pardir))

def execute(args, env=None):
  """Execute command using popen call"""
  shell = platform.system().lower() == 'windows'
  job = subprocess.Popen(args, env=env, shell=shell)
  if job.wait() == 0:
    return

  raise Exception('command execution are failed')


def sync_chromium(chromium_tag):
  """Synchronize a chromium tag"""
  print 'chromium tag: {}'.format(chromium_tag)
  execute(['git', 'fetch', '--tags'])

  # hard reset
  execute(['git', 'reset', '--hard'])

  print 'checkout ...'
  branch = 'chromium_{}'.format(chromium_tag)
  try:
    execute(['git', 'checkout', '-b', branch, chromium_tag])
  except Exception as error:
    pass

  print 'chromium sync ...'
  execute(['gclient', 'sync', '--with_branch_heads', '--jobs', '16'])


def main():
  """Main entry point"""
  # Change chromium directory src
  os.chdir(CHROME_SRC)

  chromium_tag = None
  chromium_tag_path = os.path.join(STELLITE_DIR, 'chromium.tag')
  with open(chromium_tag_path) as tagfile:
    chromium_tag = tagfile.readline().strip()

  if chromium_tag is None:
    print 'error: invalid tag file {}'.format(chromium_tag)
    sys.exit()

  sync_chromium(chromium_tag)


if __name__ == '__main__':
  main()
