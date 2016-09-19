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

import sys
import os
import platform
import subprocess

def main(argv):
  """Main entry point"""

  # Check whether node and npm exist or not
  file_path = os.path.abspath(__file__)
  root_path = os.path.dirname(os.path.dirname(file_path))
  system = platform.system().lower()

  node_path = os.path.join(root_path, system, 'bin', 'node')
  npm_path = os.path.join(root_path, system, 'bin', 'npm')
  if not os.path.exists(node_path) or not os.path.exists(npm_path):
    print 'install node, npm ...'
    # Install node and npm
    install_path = os.path.join(os.path.dirname(file_path),
                                '.'.join([system, 'sh']))
    res = subprocess.Popen([install_path]).wait()
    # TODO(@snibug): Process res

  # Install package.json dependencies
  module_path = os.path.join(root_path, 'node_modules')
  if not os.path.exists(module_path):
    print 'install node modules ...'
    os.chdir(root_path)
    res = subprocess.Popen([npm_path, 'install'])
    # TODO(@snibug): Process res


if __name__ == '__main__':
  main(sys.argv[1:])
