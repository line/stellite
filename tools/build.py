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


import argparse
import copy
import distutils.spawn
import fnmatch
import os
import platform
import subprocess
import sys
import logging

ANDROID = 'android'
ARM_VERSION = 'arm_version'
BUILD = 'build'
CHROMIUM_ANDROID_BUILD = 'chromium_android_build'
CHROMIUM_ANDROID_BUILD_BASE = 'chromium_android_build_base'
CHROMIUM_LINUX_BUILD = 'chromium_linux_build'
CHROMIUM_LINUX_BUILD_BASE = 'chromium_linux_build_base'
CLEAN = 'clean'
CLEAN_BUILD = 'clean_build'
CLIENT_BINDER = 'client_binder'
DARWIN = 'darwin'
DOCKER = 'docker'
DOCKER_MACHINE = 'docker-machine'
GYP_DEFINES = 'GYP_DEFINES'
GYP_GENERATOR_FLAGS = 'GYP_GENERATOR_FLAGS'
INFO = 'info'
IOS = 'ios'
LINUX = 'linux'
OUTPUT_DIR = 'output_dir'
PATH = 'PATH'
QUIC_SERVER = 'quic_server'
QUIC_THREAD_SERVER = 'quic_thread_server'
SHARED_LIBRARY = 'shared_library'
STATIC_LIBRARY = 'static_library'
STELLITE = 'stellite'
SYNC_CHROMIUM = 'sync_chromium'
TARGET_ARCH = 'target_arch'
TARGET_SUBARCH = 'target_subarch'
THIRD_PARTY = 'third_party'
UNITTEST = 'unittest'
WINDOWS = 'windows'

GIT_DEPOT = 'https://chromium.googlesource.com/chromium/tools/depot_tools.git'

GCLIENT_META_IOS = """
target_os = [\"ios\"]
target_os_only = \"True\"
"""

GYP_ENV_IOS = """{
  \"GYP_DEFINES\": \"OS=ios chromium_ios_signing=0\",
  \"GYP_GENERATORS\": \"ninja\",
}
"""

GCLIENT_META_ANDROID = """
target_os = [\"android\"]
target_os_only = \"True\"
"""

TARGET_ARM_V6 = 'armv6'
TARGET_ARM_V7 = 'armv7'
TARGET_ARM_64 = 'arm64'
TARGET_X86 = 'x86'
TARGET_X86_64 = 'x86_64'

ANDROID_TARGETS = {
  'armv6': {
    TARGET_ARCH: 'arm',
    ARM_VERSION: 6,
  },
  'armv7': {
    TARGET_ARCH: 'arm',
    ARM_VERSION: 7,
  },
  'arm64': {
    TARGET_ARCH: 'arm64',
    ARM_VERSION: 7,
  },
  'x86': {
    TARGET_ARCH: 'ia32',
  },
  'x86_64': {
    TARGET_ARCH: 'x64',
  },
}

IOS_TARGETS = {
  'simulator_32': {
    TARGET_ARCH: 'i386',
    TARGET_SUBARCH: 'arm32',
  },
  'simulator_64': {
    TARGET_ARCH: 'x86_64',
    TARGET_SUBARCH: 'arm64',
  },
  'device': {
    TARGET_ARCH: 'armv7',
    TARGET_SUBARCH: 'both',
  },
}

LINUX_EXCLUDE_OBJECTS = [
  'stellite_http_client_bin.http_client_bin.o',
]

ANDROID_EXCLUDE_OBJECTS = [
  'cpu_features.cpu-features.o',
  'stellite_http_client_bin.http_client_bin.o',
]

DARWIN_EXCLUDE_OBJECTS = [
  'libprotobuf_full_do_not_use.a',
  'libprotoc_lib.a',
]

SCRIPT_PATH = os.path.abspath(__file__)
TOOLS_PATH = os.path.dirname(SCRIPT_PATH)
STELLITE_PATH = os.path.dirname(TOOLS_PATH)
THIRD_PARTY_PATH = os.path.join(STELLITE_PATH, 'third_party')


def stellite_path():
  """Return stellite real path"""
  return STELLITE_PATH


def third_party_path():
  """Return third party path"""
  return THIRD_PARTY_PATH


def set_third_party_path(path):
  """set third_party path"""
  global THIRD_PARTY_PATH
  THIRD_PARTY_PATH = os.path.normpath(os.path.abspath(path))
  if not os.path.exists(THIRD_PARTY_PATH):
    make_directory(THIRD_PARTY_PATH)


def depot_tools_path():
  """Return depot_tools path"""
  return os.path.join(third_party_path(), 'depot_tools')


def execute(args, env=None, cwd=None):
  """Execute shell command"""
  # patch a depot tools directory to PATH environment
  env = env or os.environ.copy()
  env[PATH] = ''.join([depot_tools_path(), os.pathsep, env.get(PATH, '')])

  print 'command: ', ' '.join(args)
  job = subprocess.Popen(args, env=env, cwd=cwd)
  if job.wait() == 0:
    return

  raise Exception('command execution are failed')


def gyp_chromium_path(chromium_path):
  """Return gyp_chromium"""
  return os.path.join(chromium_path, 'src', 'build', 'gyp_chromium')


def gyp_path():
  """Return stellite.gyp path"""
  return os.path.join(stellite_path(), 'stellite.gyp')


def stellite_patches_path():
  """Return patches path"""
  return os.path.join(stellite_path(), 'patches')


def stellite_include_path():
  """Return Stellite include path"""
  return os.path.join(stellite_path(), 'include')


def local_platform():
  """Return system platform name"""
  return platform.system().lower()


def copy_file(target, to):
  """Copy a file"""
  execute(['cp', '-rfv', target, to])


def copy_include_file(output_path):
  """copy stellite include file"""
  copy_file(stellite_include_path(), output_path)


def make_directory(target):
  """Create a directory"""
  execute(['mkdir', '-p', target])


def remove_path(target):
  """Remove a path"""
  execute(['rm', '-rf', target])


def remove_pattern(target_dir, pattern):
  """remote a file pattern"""
  execute(['rm', '-rf', pattern], cwd=target_dir)


def host_os():
  """return host os name"""
  system_name = platform.system().lower()
  return system_name


def host_arch():
  """return host architecture"""
  return platform.uname()[4]


def ndk_absolute_root(chromium_path):
  """return android ndk path for chromium"""
  return os.path.join(chromium_path, 'src', 'third_party', 'android_tools',
                      'ndk')


def android_libcpp_root(chromium_path):
  """return android libcpp root path"""
  return os.path.join(ndk_absolute_root(chromium_path), 'sources', 'cxx-stl',
                      'llvm-libc++')


def android_app_abi(target_arch):
  """return target abi name"""
  if target_arch == TARGET_ARM_V6:
    return 'armeabi'

  if target_arch == TARGET_ARM_V7:
    return 'armeabi-v7a'

  if target_arch == TARGET_ARM_64:
    return 'arm64-v8a'

  if target_arch == TARGET_X86:
    return 'x86'

  if target_arch == TARGET_X86_64:
    return 'x86_64'

  raise Exeption('unknown target architecture: {}'.format(target_arch))


def android_toolchain_relative(target_arch):
  """select android toolchain for target archetecture"""
  if target_arch in (TARGET_ARM_V6, TARGET_ARM_V7):
    return 'arm-linux-androideabi-4.9'

  if target_arch == TARGET_ARM_64:
    return 'aarch64-linux-android-4.9'

  if target_arch == TARGET_X86:
    return 'x86-4.9'

  if target_arch == TARGET_X86_64:
    return 'x86_64-4.9'

  raise Exception('unknown target architecture error: {}'.format(target_arch))


def android_libcpp_libs_dir(target_arch, chromium_path):
  """return libcpp library path"""
  return os.path.join(android_libcpp_root(chromium_path), 'libs',
                      android_app_abi(target_arch))


def android_ndk_sysroot(target_arch, chromium_path):
  """return androdi sysroot path"""
  if target_arch in (TARGET_ARM_V6, TARGET_ARM_V7):
    return os.path.join(ndk_absolute_root(chromium_path), 'platforms',
                        'android-16', 'arch-arm')

  if target_arch == TARGET_ARM_64:
    return os.path.join(ndk_absolute_root(chromium_path), 'platforms',
                        'android-21', 'arch-arm64')

  if target_arch == TARGET_X86:
    return os.path.join(ndk_absolute_root(chromium_path), 'platforms',
                        'android-16', 'arch-x86')

  if target_arch == TARGET_X86_64:
    return os.path.join(ndk_absolute_root(chromium_path), 'platforms',
                        'android-21', 'arch-x86_64')

  raise Exception('unknown target error')


def android_ndk_lib_dir(target_arch):
  """return android ndk relative lib"""
  if target_arch in (TARGET_ARM_V6, TARGET_ARM_V7, TARGET_ARM_64, TARGET_X86):
    return os.path.join('usr', 'lib')

  if target_arch == TARGET_X86_64:
    return os.path.join('usr', 'lib64')

  raise Exception('unknown target architecture error: {}'.format(target_arch))


def android_ndk_lib(target_arch, chromium_path):
  """return ndk lib directory"""
  return os.path.join(android_ndk_sysroot(target_arch, chromium_path),
                      android_ndk_lib_dir(target_arch))


def android_toolchain(target_arch, chromium_path):
  """return android build toolchain path"""
  return os.path.join(ndk_absolute_root(chromium_path), 'toolchains',
                      android_toolchain_relative(target_arch), 'prebuilt',
                      '{}-{}'.format(host_os(), host_arch()), 'bin')


def android_compiler_path(target_arch, chromium_path):
  """return android toolchain's g++ compiler path."""
  toolchain_path = android_toolchain(target_arch, chromium_path)
  path_list = filter(lambda x: x.endswith('g++'), os.listdir(toolchain_path))
  if len(path_list) < 1:
    raise Exception('invalid toolchain directory error: ' + toolchain_path)
  return os.path.join(toolchain_path, path_list[0])


def android_ar_path(target_arch, chromium_path):
  """return android toolchain's ar path"""
  toolchain_path = android_toolchain(target_arch, chromium_path)
  path_list = filter(lambda x: x.endswith('ar'), os.listdir(toolchain_path))
  if len(path_list) < 1:
    raise Exception('invalid toolchain directory error' + toolchain_path)
  return os.path.join(toolchain_path, path_list[0])


def android_libgcc_filename(target_arch, chromium_path):
  """return android-gcc -print-libgcc-file-name execution result"""
  toolchain_path = android_toolchain(target_arch, chromium_path)
  path_list = filter(lambda x: x.endswith('gcc'), os.listdir(toolchain_path))
  if len(path_list) < 1:
    raise Exception('invalid toolchain directory error: ' + toolchain_path)
  gcc_path = os.path.join(toolchain_path, path_list[0])
  return read_execute_stdout([gcc_path, '-print-libgcc-file-name'])


def binutils_path(chromium_path):
  """x64 binutils path"""
  return os.path.join(chromium_path, 'src', 'third_party', 'binutils',
                      'Linux_x64', 'Release', 'bin')


def clang_dir(chromium_path):
  """return clang dir"""
  return os.path.join(chromium_path, 'src', 'third_party', 'llvm-build',
                      'Release+Asserts')


def clang_compiler_path(chromium_path):
  """return ios clang compiler path"""
  return os.path.join(clang_dir(chromium_path), 'bin', 'clang++')


def sdk_path(target):
  """reutrn xcode sdk path"""
  sdk_command = ['xcrun', '-sdk', target, '--show-sdk-path']
  job = subprocess.Popen(sdk_command, stdout=subprocess.PIPE)
  job.wait()
  stdout = job.stdout.read()
  return stdout.strip()


def iphone_sdk_path():
  """return iphoneos sdk path"""
  return sdk_path('iphoneos')


def simulator_sdk_path():
  """return simulator sdk path"""
  return sdk_path('iphonesimulator')


def mac_sdk_path():
  """return mac sdk path"""
  return sdk_path('macosx')


def ninja_target_name(target):
  if target == STELLITE:
    return 'stellite_http_client_bin'
  if target == QUIC_SERVER:
    return 'quic_thread_server'
  if target == CLIENT_BINDER:
    return 'stellite_client_binder'
  raise Exception('unknown target error: {}'.format(target))


def target_chromium_path(target):
  """Return target chromium path that was fetched from third_part directory"""
  if not target in (LINUX, ANDROID, IOS, DARWIN):
    raise Exception('invalid target platform: {}'.format(target))

  return os.path.join(third_party_path(), 'chromium_{}'.format(target))


def which_application(name):
  """Find whether the application exists in the current environment or not"""
  return distutils.spawn.find_executable(name)


def read_chromium_tag():
  """Read src/chromium.tag"""
  chromium_tag_file_path = os.path.join(stellite_path(), 'chromium.tag')
  if not os.path.exists(chromium_tag_file_path):
    raise Exception('chromium.tag is not exist error')

  with open(chromium_tag_file_path, 'r') as tag_file:
    return tag_file.read().strip()


def read_chromium_branch(chromium_path):
  """Read chromium git branch"""
  git_command = ['git', 'rev-parse', '--abbrev-ref', 'HEAD']
  working_path = os.path.join(chromium_path, 'src')
  job = subprocess.Popen(git_command, cwd=working_path, stdout=subprocess.PIPE)
  job.wait()
  stdout = job.stdout.read()
  return stdout.strip()


def read_execute_stdout(args, env=None):
  """execute command line argument and return a result of stdout stream"""
  res = subprocess.Popen(args, env=env, stdout=subprocess.PIPE)
  if bool(res.wait()):
    traceback.print_stack()
    log.error(' '.join(args) + ' was failed')
    sys.exit()

  stdout = res.stdout.read()
  return stdout.strip()


def is_chromium_path(chromium_path):
  """Check whether it is chromium or not"""
  if not os.path.exists(chromium_path):
    return False

  return os.path.exists(os.path.join(chromium_path, '.gclient'))


def write_gclient_meta_info(chromium_path, meta_info):
  """Write target OS meta info to .gclient file"""
  if not is_chromium_path(chromium_path):
    raise Exception('invalid chromium path error')

  gclient_meta_path = os.path.join(chromium_path, '.gclient')
  with open(gclient_meta_path, 'r') as gclient_meta_file:
    gclient_context = gclient_meta_file.read()
    if 'target_os' in gclient_context:
      return False

  with open(gclient_meta_path, 'a') as gclient_meta_file:
    gclient_meta_file.write(meta_info)

  return True


def write_gyp_env(chromium_path, gyp_env):
  """Write chromium.gyp_env file"""
  chromium_gyp_env_path = os.path.join(chromium_path, 'chromium.gyp_env')
  with open(chromium_gyp_env_path, 'w') as chromium_gyp_env_file:
    chromium_gyp_env_file.write(gyp_env)


def patch_git_apply(working_path, patch_file_path, target_dir=None):
  """"""
  if not os.path.exists(patch_file_path):
    raise Exception('{} is not exist error'.format(patch_file_path))

  patch_command = ['git', 'apply', '--ignore-space-change']
  if target_dir:
    patch_command.extend(['--directory', target_dir])
  patch_command.append(patch_file_path)

  try:
    execute(patch_command, cwd=working_path)
  except Exception:
    pass


def patch_boringssl(chromium_path):
  """Patch BoringSSL from stellite/patches/boringssl_gypi.patch"""
  patch_file_path = os.path.join(stellite_patches_path(), 'boringssl.diff')
  chromium_src_path = os.path.join(chromium_path, 'src')
  patch_git_apply(chromium_src_path, patch_file_path)


def patch_diff(chromium_path, diff_file):
  """patch diff file on stellite/patches"""
  patch_file_path = os.path.join(stellite_patches_path(), diff_file)
  chromium_src_path = os.path.join(chromium_path, 'src')
  patch_git_apply(chromium_src_path, patch_file_path)


def patch_build_script(chromium_path):
  """Patch build/all.gyp to global build"""
  patch_file_path = os.path.join(stellite_patches_path(), 'build_gyp.diff')
  chromium_src_path = os.path.join(chromium_path, 'src')
  patch_git_apply(chromium_src_path, patch_file_path)

  chromium_stellite_path = os.path.join(chromium_src_path, 'stellite')
  chromium_stellite_path = os.path.abspath(os.path.normpath(
      chromium_stellite_path))

  if chromium_stellite_path == stellite_path():
    return

  if os.path.exists(chromium_stellite_path):
    remove_path(chromium_stellite_path)
  make_directory(chromium_stellite_path)

  for target_path in os.listdir(stellite_path()):
    if THIRD_PARTY in target_path:
      continue
    copy_file(os.path.join(stellite_path(), target_path),
              chromium_stellite_path)


def fetch_depot_tools():
  """Check depot_tools"""
  tools_path = depot_tools_path()
  if os.path.exists(tools_path):
    return tools_path
  execute(['git', 'clone', GIT_DEPOT, tools_path])
  execute(['git', 'checkout', 'abb9b22752a6ddd8a996c9198c06060b132ba5f0'],
          cwd=tools_path)
  return tools_path


def sync_chromium_tag(chromium_path):
  """Sync chromium with stellite/chromium.tag"""
  target_chromium_tag = read_chromium_tag()
  current_chromium_branch = read_chromium_branch(chromium_path)
  if target_chromium_tag in current_chromium_branch:
    logging.info('chromium {} already sync'.format(target_chromium_tag))
    return False

  working_path = os.path.join(chromium_path, 'src')
  execute(['git', 'fetch', '--tags'], cwd=working_path)
  execute(['git', 'reset', '--hard'], cwd=working_path)

  branch = 'chromium_{}'.format(target_chromium_tag)
  execute(['git', 'checkout', '-b', branch, target_chromium_tag],
          cwd=working_path)

  gclient_path = os.path.join(depot_tools_path(), 'gclient')
  execute([gclient_path, 'sync', '--with_branch_heads', '--jobs', '16'],
          cwd=working_path)

  return True


def gclient_sync_chromium(chromium_path):
  """Execute $(depot_tools)/gclient sync"""
  gclient_path = os.path.join(depot_tools_path(), 'gclient')
  command = [gclient_path, 'sync']
  execute(command, cwd=chromium_path)


def fetch_chromium(chromium_path):
  """Fetch a chromium code using depot-tools/fetch.py"""
  if os.path.exists(chromium_path) and is_chromium_path(chromium_path):
    return chromium_path

  if os.path.exists(chromium_path):
    remove_path(chromium_path)

  make_directory(chromium_path)

  fetch_path = os.path.join(depot_tools_path(), 'fetch')
  fetch_command = [fetch_path, '--nohooks', 'chromium']
  execute(fetch_command, cwd=chromium_path)
  return chromium_path


def fetch_target_platform_toolchain(chromium_path, target_platform):
  """Fetch target platform tools in chromium"""
  if not target_platform in (IOS, ANDROID):
    return

  if target_platform == IOS:
    write_gclient_meta_info(chromium_path, GCLIENT_META_IOS)
    write_gyp_env(chromium_path, GYP_ENV_IOS)
  elif target_platform == ANDROID:
    write_gclient_meta_info(chromium_path, GCLIENT_META_ANDROID)

  gclient_sync_chromium(chromium_path)


def fetch_target_chromium(target_platform):
  """Fetch target chromium(Linux, MAC, iOS, Android)"""
  if not target_platform in (LINUX, ANDROID, IOS, DARWIN):
    raise Exception('invalid target platform')

  chromium_path = target_chromium_path(target_platform)
  if os.path.exists(chromium_path) and is_chromium_path(chromium_path):
    return chromium_path

  system = platform.system().lower()

  if target_platform == LINUX:
    if system != LINUX:
      raise Exception('fetching linux chromium, must on linux system')

  elif target_platform == ANDROID:
    kernel = platform.uname()[3].lower()
    if not 'ubuntu' in kernel:
      raise Exception('android build are must on ubuntu system')

  elif target_platform == IOS:
    if system != DARWIN:
      raise Exception('ios build are must on drawin system')

  elif target_platform == DARWIN:
    if system != DARWIN:
      raise Exception('darwin build are must on darwin system')

  fetch_chromium(chromium_path)
  fetch_target_platform_toolchain(chromium_path, target_platform)
  sync_chromium_tag(chromium_path)

  return chromium_path


def linux_link_static_library(build_path, exclude_objects, library_name):
  """Link static library"""
  library_path = os.path.join(build_path, 'lib{}.a'.format(library_name))
  link_command = ['ar', 'rsc', library_path]

  for path, dir_name, file_list in os.walk(build_path):
    for matched in fnmatch.filter(file_list, '*.o'):
      if matched in exclude_objects:
        continue
      link_command.append(os.path.join(path, matched))
  execute(link_command)
  return library_path


def android_link_library(target_arch, chromium_path, build_path,
                         exclude_objects, library_name,
                         target_type=STATIC_LIBRARY):
  """link android library"""
  if target_type == STATIC_LIBRARY:
    return android_link_static_library(target_arch, chromium_path, build_path,
                                       exclude_objects, library_name)

  if target_type == SHARED_LIBRARY:
    return android_link_shared_library(target_arch, chromium_path, build_path,
                                       exclude_objects, library_name)

  raise Exception('unknown target_type error: {}'.format(target_type))


def android_link_static_library(target_arch, chromium_path, build_path,
                                exclude_objects, library_name):
  """link static library"""
  target_library_name = 'lib{}.a'.format(library_name)
  target_library_path = os.path.join(build_path, target_library_name)
  command = [
    android_ar_path(target_arch, chromium_path),
    'rsc', target_library_path,
  ]

  for path, dir_name, file_list in os.walk(build_path):
    for matched in fnmatch.filter(file_list, '*.o'):
      if matched in exclude_objects:
        continue
      command.append(os.path.join(path, matched))
  execute(command)
  return target_library_path


def android_link_shared_library(target_arch, chromium_path, build_path,
                                exclude_objects, library_name):
  """link android shared library"""
  library_file_name = 'lib{}.so'.format(library_name)
  library_file_path = os.path.join(build_path, library_file_name)
  command = [
    android_compiler_path(target_arch, chromium_path),
    '-Wl,-shared',
    '-Wl,-z,now',
    '-Wl,-z,relro',
    '-Wl,--fatal-warnings',
    '-Wl,-z,defs',
    '-Wl,-z,noexecstack',
    '-fPIC',
    '-B{}'.format(binutils_path(chromium_path)),
    '-Wl,-z,relro',
    '-Wl,-z,now',
    '-fuse-ld=gold',
    '-Wl,--build-id=sha1',
    '-Wl,--no-undefined',
    '--sysroot={}'.format(android_ndk_sysroot(target_arch, chromium_path)),
    '-nostdlib',
    '-L{}'.format(android_libcpp_libs_dir(target_arch, chromium_path)),
    '-Wl,--exclude-libs=libgcc.a',
    '-Wl,--exclude-libs=libc++_static.a',
    '-Wl,--exclude-libs=libcommon_audio.a',
    '-Wl,--exclude-libs=libcommon_audio_neon.a',
    '-Wl,--exclude-libs=libcommon_audio_sse2.a',
    '-Wl,--exclude-libs=libiSACFix.a',
    '-Wl,--exclude-libs=libisac_neon.a',
    '-Wl,--exclude-libs=libopus.a',
    '-Wl,--exclude-libs=libvpx.a',
    '-Wl,--icf=all',
    '-Wl,-Bsymbolic',
    '-Wl,--gc-sections',
    '-Wl,-z,nocopyreloc',
    # crtbegin_so.o need to be the first item in library object
    '{}/{}'.format(android_ndk_lib(target_arch, chromium_path),
                   'crtbegin_so.o'),
    '-Wl,-O1',
    '-Wl,--as-needed',
    '-Wl,--warn-shared-textrel',
    '-o', library_file_path
  ]

  command.append('-Wl,--start-group')
  command.extend(all_files(os.path.join(build_path, 'obj'), '*.o',
                           exclude_objects))
  command.append('-Wl,--end-group')

  # caution: android_libgcc library must after of libc++_static
  command.extend([
    '-Wl,-wrap,calloc',
    '-Wl,-wrap,free',
    '-Wl,-wrap,malloc',
    '-Wl,-wrap,memalign',
    '-Wl,-wrap,posix_memalign',
    '-Wl,-wrap,pvalloc',
    '-Wl,-wrap,realloc',
    '-Wl,-wrap,valloc',
    '-lc++_static',
    android_libgcc_filename(target_arch, chromium_path),
    '-latomic',
    '-lc',
    '-ldl',
    '-lm',
    '-llog',
    # do not add any libraries after this!
    '{}/{}'.format(android_ndk_lib(target_arch, chromium_path), 'crtend_so.o')
  ])

  execute(command)
  return library_file_path


def ios_merge_fat_library(library_path_list, output_library_path):
  """merge fat library"""
  merge_command = ['lipo', '-create']
  merge_command.extend(library_path_list)

  merge_command.append('-output')
  merge_command.append(output_library_path)

  execute(merge_command)
  return output_library_path


def ios_link_shared_library(target_arch_list, chromium_path, build_path,
                            library_name):
  """link ios dynamic libray and return path"""
  if len(target_arch_list) == 0:
    raise Exception('invalid target architecture input error')

  merged_library_name = 'lib{}.dylib'.format(library_name)

  res_libs = []
  for target_arch in target_arch_list:
    sdk_path = iphone_sdk_path()
    target_archive_path = build_path
    target_archive_pattern = '*.a'

    if not 'arm' in target_arch:
      sdk_path = simulator_sdk_path()

    if os.path.exists(os.path.join(build_path, 'arch')):
      target_archive_path = os.path.join(build_path, 'arch')
      target_archive_pattern = '*{}.a'.format(target_arch)

    command = [
      clang_compiler_path(chromium_path),
      '-shared',
      '-Wl,-search_paths_first',
      '-Wl,-dead_strip',
      '-miphoneos-version-min=9.0',
      '-isysroot', sdk_path,
      '-arch', target_arch,
      '-install_name', '@loader_path/{}'.format(merged_library_name),
    ]
    for filename in all_files(target_archive_path, target_archive_pattern):
      command.append('-Wl,-force_load,{}'.format(filename))

    target_arch_library_name = 'lib{}.{}.dylib'.format(library_name,
                                                       target_arch)
    target_arch_lib_path = os.path.join(build_path, target_arch_library_name)
    command.extend([
      '-o', target_arch_lib_path,
      '-stdlib=libc++',
      '-lresolv',
      '-framework', 'CFNetwork',
      '-framework', 'CoreFoundation',
      '-framework', 'CoreGraphics',
      '-framework', 'CoreText',
      '-framework', 'Foundation',
      '-framework', 'MobileCoreServices',
      '-framework', 'Security',
      '-framework', 'SystemConfiguration',
      '-framework', 'UIKit',
    ])

    execute(command)
    res_libs.append(target_arch_lib_path)

  # merge fat library
  merged_library_path = os.path.join(build_path, merged_library_name)
  return ios_merge_fat_library(res_libs, merged_library_path)


def ios_link_static_library(target_arch_list, chromium_path, build_path,
                            library_name):
  """link object file to static library"""
  libtool_path = which_application('libtool')
  if not libtool_path:
    raise Exception('libtool is not exist error')

  gyp_mac_tool_path = os.path.join(build_path, 'gyp-mac-tool')
  target_library_name = 'lib{}.a'.format(library_name)
  library_path = os.path.join(build_path, target_library_name)

  link_command = [gyp_mac_tool_path, 'filter-libtool', libtool_path,
                  '-static', '-o', library_path]

  for matched in fnmatch.filter(os.listdir(build_path), '*.a'):
    link_command.append(os.path.join(build_path, matched))

  execute(link_command)
  return library_path


def ios_link_library(target_arch_list, chromium_path, build_path, library_name,
                     target_type=STATIC_LIBRARY):
  """link ios library"""
  if target_type == STATIC_LIBRARY:
    return ios_link_static_library(target_arch_list, chromium_path, build_path,
                                   library_name)

  if target_type == SHARED_LIBRARY:
    return ios_link_shared_library(target_arch_list, chromium_path, build_path,
                                   library_name)

  raise Exception('invalid target type error: {}'.format(target_type))


def darwin_link_library(chromium_path, build_path, library_name,
                        target_type=STATIC_LIBRARY):
  """link darwin library"""
  if target_type == STATIC_LIBRARY:
    return darwin_link_static_library(build_path, library_name)

  if target_type == SHARED_LIBRARY:
    return darwin_link_shared_library(chromium_path, build_path, library_name)

  raise Exception('unknown target type error: {}'.format(target_type))


def darwin_link_shared_library(chromium_path, build_path, library_name):
  """link ios shared libray and return path"""
  command = [
    clang_compiler_path(chromium_path),
    '-shared',
    '-Wl,-search_paths_first',
    '-Wl,-dead_strip',
    '-isysroot', mac_sdk_path(),
    '-arch', 'x86_64',
  ]

  for filename in all_files(build_path, '*.a', DARWIN_EXCLUDE_OBJECTS):
    command.append('-Wl,-force_load,{}'.format(filename))

  target_library_name = 'lib{}.dylib'.format(library_name)
  target_library_path = os.path.join(build_path, target_library_name)
  command.extend([
    '-o', target_library_path,
    '-install_name', '@loader_path/{}'.format(target_library_name),
    '-stdlib=libc++',
    '-lresolv',
    '-lbsm',
    '-framework', 'AppKit',
    '-framework', 'ApplicationServices',
    '-framework', 'Carbon',
    '-framework', 'CoreFoundation',
    '-framework', 'Foundation',
    '-framework', 'IOKit',
    '-framework', 'Security',
    '-framework', 'SystemConfiguration'
  ])

  execute(command)
  return target_library_path


def darwin_link_static_library(build_path, library_name):
  """Link static library"""
  libtool_path = which_application('libtool')
  if not libtool_path:
    raise Exception('libtool is not exist error')

  gyp_mac_tool_path = os.path.join(build_path, 'gyp-mac-tool')

  target_library_name = 'lib{}.a'.format(library_name)
  target_library_path = os.path.join(build_path, target_library_name)
  link_command = [gyp_mac_tool_path, 'filter-libtool', libtool_path,
                  '-static', '-o', target_library_path]

  for filename in all_files(build_path, '*.a', DARWIN_EXCLUDE_OBJECTS):
    link_command.append(filename)

  execute(link_command)
  return target_library_path


def docker_environment():
  """Return map of docker environment for executing a docker"""
  if local_platform() != DARWIN:
    return os.environ.copy()

  if not which_application(DOCKER):
    raise Exception('{} is not exist error'.format(DOCKER))

  job = subprocess.Popen([DOCKER, INFO], stdout=subprocess.PIPE)
  job.wait()

  stdout = job.stdout.read()
  env = os.environ.copy()
  if len(stdout) == 0:
    if not which_application(DOCKER_MACHINE):
      raise Exception('{} is not exist error'.format(DOCKER_MACHINE))

    job = subprocess.Popen([DOCKER_MACHINE, 'env', 'default'],
                           stdout=subprocess.PIPE)
    job.wait()
    stdout = job.stdout.read()
    if len(stdout) == 0:
      raise Exception('docker is not running')

    for export_command in stdout.splitlines():
      export_pair = export_command.split()
      if len(export_pair) != 2:
        continue

      if not '=' in export_pair[1]:
        continue

      item_key, item_value = export_pair[1].split('=')
      env[item_key] = item_value.replace('"', '')

  return env


def find_docker_image(image_name, env=None):
  """Check whether the docker image exists or not"""
  if not which_application(DOCKER):
    raise Exception('{} is not exist error'.format(DOCKER))

  command_line = [DOCKER, 'images', str(image_name)]
  job = subprocess.Popen(command_line, env=env, stdout=subprocess.PIPE)
  stdout, _ = job.communicate()
  return stdout.count('\n') > 1


def normalize_gyp_defines(gyp_defines):
  """Normalize a gyp_defines dictionary"""
  defines = []
  for key, value in gyp_defines.iteritems():
    defines.append('{}={}'.format(str(key), str(value)))
  return ' '.join(defines)


def ninja_build(target, output_path):
  """Execute Ninja build command"""
  command = ['ninja', '-C', output_path, target]
  execute(command)


def all_files(build_path, pattern, filter_patterns=None):
  """return target object list from ninja build's output"""
  res = []
  filter_patterns = filter_patterns or []
  for path, dirname, file_list in os.walk(build_path):
    for matched in fnmatch.filter(file_list, pattern):
      if matched in filter_patterns:
        continue
      res.append(os.path.join(path, matched))
  return res


def generate_build_script(chromium_path, gyp_defines=None, build_path=None,
                          using_stellite_gyp=True):
  """Generate Ninja build script"""
  env = os.environ.copy()
  if gyp_defines:
    env[GYP_DEFINES] = normalize_gyp_defines(gyp_defines)

  build_path = build_path or os.path.join(chromium_path, 'src', 'out')
  gyp_generator_flags = {
    OUTPUT_DIR: build_path,
  }
  env[GYP_GENERATOR_FLAGS] = normalize_gyp_defines(gyp_generator_flags)

  gyp_chromium = gyp_chromium_path(chromium_path)
  command = [gyp_chromium]
  if using_stellite_gyp:
    stellite_gyp = gyp_path()
    if not os.path.exists(gyp_chromium):
      raise Exception('gyp_chromium is not exist: chromium path is not valid')
    command.append(stellite_gyp)
  else:
    patch_build_script(chromium_path)

  chromium_src_path = os.path.join(chromium_path, 'src')
  command.append('--depth={}'.format(chromium_src_path))
  execute(command, env=env, cwd=chromium_path)


def darwin_ios_stellite_build(output_path, target_type=STATIC_LIBRARY,
                              chromium_path=None):
  """build stellite client dynamic library"""
  if not chromium_path:
    chromium_path = fetch_target_chromium(IOS)
  chromium_path = os.path.normpath(os.path.abspath(chromium_path))

  gyp_defines = {
    'OS': 'ios',
    'component': 'static_library',
    'fastbuild': 2,
    'clang': 1,
    'disable_ftp_support': 1,
    'chromium_ios_signing': 0,
  }

  build_base_path = os.path.join(chromium_path, 'src')
  all_libs = []
  for target_name, target_info in IOS_TARGETS.iteritems():
    platform_gyp_defines = copy.deepcopy(gyp_defines)
    platform_gyp_defines.update(target_info)

    target_build_path = os.path.join(build_base_path,
                                     'out_ios_{}'.format(target_name))
    if not os.path.exists(target_build_path):
      patch_diff(chromium_path, 'boringssl.diff')
      generate_build_script(chromium_path, platform_gyp_defines,
                            target_build_path, using_stellite_gyp=False)

    release_directory = None
    target_arch_list = []
    if 'simulator_32' in target_name:
      release_directory = 'Release-iphonesimulator'
      target_arch_list = ['i386']
    elif 'simulator_64' in target_name:
      release_directory = 'Release-iphonesimulator'
      target_arch_list = ['x86_64']
      # TODO(@snibug): xcode6 and xcode7 has no comparability for building ios
      # simulator x64 on dynamic library link. there are anyone to solving this?
      if target_type == SHARED_LIBRARY:
        continue
    elif 'device' in target_name:
      release_directory = 'Release-iphoneos'
      target_arch_list = ['armv7', 'arm64']
    else:
      raise Exception('unknown target platform error')

    platform_release_build_path = os.path.join(target_build_path,
                                               release_directory)
    remove_pattern(platform_release_build_path, '*.a')

    ninja_target = ninja_target_name(STELLITE)
    ninja_build(ninja_target, platform_release_build_path)
    lib_path = ios_link_library(target_arch_list, chromium_path,
                                platform_release_build_path, STELLITE,
                                target_type=target_type)
    platform_output_path = os.path.join(output_path,
                                        'ios_{}'.format(target_name))
    if os.path.exists(platform_output_path):
      remove_path(platform_output_path)
    make_directory(platform_output_path)

    copy_file(lib_path, platform_output_path)

    all_libs.append(lib_path)

  # merge single simulator dynamic library
  ios_output_path = os.path.join(output_path, 'ios_output')
  if os.path.exists(ios_output_path):
    remove_path(ios_output_path)
  make_directory(ios_output_path)

  ios_fat_library_name = 'lib{}'.format(STELLITE)
  if target_type == STATIC_LIBRARY:
    ios_fat_library_name += '.a'
  else:
    ios_fat_library_name += '.dylib'

  ios_fat_library_path = os.path.join(ios_output_path, ios_fat_library_name)
  ios_merge_fat_library(all_libs, ios_fat_library_path)
  copy_include_file(ios_output_path)


def darwin_stellite_build(output_path, chromium_path=None,
                          target_type=STATIC_LIBRARY):
  """"""
  if not chromium_path:
    chromium_path = fetch_target_chromium(DARWIN)
  chromium_path = os.path.abspath(os.path.normpath(chromium_path))

  gyp_defines = {
    'os': 'mac',
    'clang': 1,
    'disable_ftp_support': 1,
    'fastbuild': 2,
    'target_arch': 'x64',
  }
  build_base_path = os.path.join(chromium_path, 'src')

  platform_build_path = os.path.join(build_base_path, 'out_darwin')
  if not os.path.exists(platform_build_path):
    patch_diff(chromium_path, 'boringssl.diff')
    generate_build_script(chromium_path, gyp_defines, platform_build_path)

  platform_output_path = os.path.join(output_path, 'darwin')
  if os.path.exists(platform_output_path):
    remove_path(platform_output_path)
  make_directory(platform_output_path)

  platform_release_build_path = os.path.join(platform_build_path, 'Release')
  ninja_build(ninja_target_name(STELLITE), platform_release_build_path)

  lib_path = darwin_link_library(chromium_path, platform_release_build_path,
                                 STELLITE, target_type=target_type)
  copy_file(lib_path, platform_output_path)

  copy_include_file(platform_output_path)


def darwin_quic_server_build(output_path, chromium_path=None):
  """"""
  if not chromium_path:
    chromium_path = fetch_target_chromium(DARWIN)
  chromium_path = os.path.abspath(os.path.normpath(chromium_path))

  gyp_defines = {
    'os': 'mac',
    'clang': 1,
    'disable_ftp_support': 1,
    'fastbuild': 2,
    'target_arch': 'x64',
  }
  build_base_path = os.path.join(chromium_path, 'src')

  platform_build_path = os.path.join(build_base_path, 'out_darwin')
  if not os.path.exists(platform_build_path):
    generate_build_script(chromium_path, gyp_defines, platform_build_path)

  platform_output_path = os.path.join(output_path, 'darwin')
  if os.path.exists(platform_output_path):
    remove_path(platform_output_path)
  make_directory(platform_output_path)

  platform_release_build_path = os.path.join(platform_build_path, 'Release')
  ninja_build(QUIC_THREAD_SERVER, platform_release_build_path)
  copy_file(os.path.join(platform_release_build_path, QUIC_THREAD_SERVER),
            platform_output_path)


def darwin_linux_build(target, output_path, chromium_path=None,
                       target_type=STATIC_LIBRARY):
  """Linux build on Darwin platform"""
  if not which_application(DOCKER):
    raise Exception('docker is not exist error')

  if chromium_path:
    logging.warning('target build chromium with dorcker image, --chromium-path '
                    'was ignored')

  docker_env = docker_environment()
  if not find_docker_image(CHROMIUM_LINUX_BUILD_BASE):
    dockerfile_path = os.path.join(stellite_path(), 'docker',
                                   'linux_base')
    command = [DOCKER, 'build', '-t', CHROMIUM_LINUX_BUILD_BASE,
               dockerfile_path]
    execute(command, env=docker_env)

  chromium_tag = read_chromium_tag()
  build_image = '{}:{}'.format(CHROMIUM_LINUX_BUILD, chromium_tag)
  if not find_docker_image(build_image):
    dockerfile_path = os.path.join(stellite_path(), 'docker',
                                   'linux_tag')
    branch_name = 'chromium_{}'.format(chromium_tag)
    command = [DOCKER, 'build', '-t', build_image,
               '--build-arg', 'BRANCH={}'.format(branch_name),
               '--build-arg', 'CHROMIUM_TAG={}'.format(chromium_tag),
               dockerfile_path]
    execute(command, env=docker_env)

  container_stellite_path = '/root/chromium/src/stellite'
  container_build_script_path = os.path.join(container_stellite_path,
                                             'docker', 'bin')
  build_command = [
    'docker', 'run',
    '--rm',
    '-v', '{}:{}'.format(stellite_path(), container_stellite_path),
    '-v', '{}:{}'.format(output_path, '/out'),
    '-e', '{}={}'.format('OUT', '/out'),
    '-e', '{}={}'.format('CHROMIUM_PATH', '/root/chromium'),
    '-e', '{}={}'.format('TARGET', target),
    '-e', '{}={}'.format('ACTION', 'build'),
    '-e', '{}={}'.format('TARGET_TYPE', target_type),
    '-t', build_image,
    os.path.join(container_build_script_path, 'build_linux.sh'),
  ]

  execute(build_command, env=docker_env)


def darwin_android_build(target, output_path, chromium_path=None,
                         target_type=STATIC_LIBRARY):
  """Android build on Darwin platform"""
  if not which_application(DOCKER):
    raise Exception('docker is not exist error')

  if chromium_path:
    logging.warning('target build chromium with dorcker image, --chromium-path '
                    'was ignored')

  docker_env = docker_environment()
  if not find_docker_image(CHROMIUM_ANDROID_BUILD_BASE):
    dockerfile_path = os.path.join(stellite_path(), 'docker',
                                   'android_base')
    command = [DOCKER, 'build', '-t', CHROMIUM_ANDROID_BUILD_BASE,
               dockerfile_path]
    execute(command, env=docker_env)

  chromium_tag = read_chromium_tag()
  build_image = '{}:{}'.format(CHROMIUM_ANDROID_BUILD, chromium_tag)
  if not find_docker_image(build_image):
    dockerfile_path = os.path.join(stellite_path(), 'docker',
                                   'android_tag')
    branch_name = 'chromium_{}'.format(chromium_tag)
    command = [DOCKER, 'build', '-t', build_image,
               '--build-arg', 'BRANCH={}'.format(branch_name),
               '--build-arg', 'CHROMIUM_TAG={}'.format(chromium_tag),
               dockerfile_path]
    execute(command, env=docker_env)

  container_stellite_path = '/root/chromium/src/stellite'
  container_build_script_path = os.path.join(container_stellite_path,
                                             'docker', 'bin')
  build_command = [
    'docker', 'run',
    '--rm',
    '-v', '{}:{}'.format(stellite_path(), container_stellite_path),
    '-v', '{}:{}'.format(output_path, '/out'),
    '-e', '{}={}'.format('OUT', '/out'),
    '-e', '{}={}'.format('CHROMIUM_PATH', '/root/chromium'),
    '-e', '{}={}'.format('TARGET', target),
    '-e', '{}={}'.format('ACTION', 'build'),
    '-e', '{}={}'.format('TARGET_TYPE', target_type),
    '-t', build_image,
    os.path.join(container_build_script_path, 'build_android.sh'),
  ]

  execute(build_command, env=docker_env)


def darwin_ios_build(target, output_path, chromium_path=None,
                     target_type=STATIC_LIBRARY):
  """"""
  if target == STELLITE:
    darwin_ios_stellite_build(output_path, chromium_path=chromium_path,
                              target_type=target_type)
    return

  raise Exception('unsupported target exception: {}'.format(target))


def darwin_build(target, target_platform, output_path,
                 chromium_path=None, target_type=STATIC_LIBRARY):
  """"""
  if not target_platform in (IOS, DARWIN, LINUX, ANDROID):
    raise Exception('unsupported --target-platform error')

  if target_platform == LINUX:
    darwin_linux_build(target, output_path, target_type=target_type,
                       chromium_path=chromium_path)
    return

  if target_platform == ANDROID:
    darwin_android_build(target, output_path, target_type=target_type,
                         chromium_path=chromium_path)
    return

  if target_platform == IOS:
    darwin_ios_build(target, output_path, target_type=target_type,
                     chromium_path=chromium_path)
    return

  if target == STELLITE:
    darwin_stellite_build(output_path, target_type=target_type,
                          chromium_path=chromium_path)
    return

  if target == QUIC_SERVER:
    darwin_quic_server_build(output_path, chromium_path=chromium_path)
    return

  raise Exception('unsupport target error: {}'.format(target))


def docker_android_build(target, output_path, chromium_path=None,
                         target_type=STATIC_LIBRARY):
  """"""
  if not which_application(DOCKER):
    raise Exception('docker is not exist error')

  if chromium_path:
    logging.warning('target build chromium with dorcker image, --chromium-path '
                    'was ignored')

  if not find_docker_image(CHROMIUM_ANDROID_BUILD_BASE):
    dockerfile_path = os.path.join(stellite_path(), 'docker',
                                   'android_base')
    command = [DOCKER, 'build', '-t', CHROMIUM_ANDROID_BUILD_BASE,
               dockerfile_path]
    execute(command)

  chromium_tag = read_chromium_tag()
  build_image = '{}:{}'.format(CHROMIUM_ANDROID_BUILD, chromium_tag)
  if not find_docker_image(build_image):
    dockerfile_path = os.path.join(stellite_path(), 'docker',
                                   'android_tag')
    branch_name = 'chromium_{}'.format(chromium_tag)
    command = [DOCKER, 'build', '-t', build_image,
               '--build-arg', 'BRANCH={}'.format(branch_name),
               '--build-arg', 'CHROMIUM_TAG={}'.format(chromium_tag),
               dockerfile_path]
    execute(command)

  container_stellite_path = '/root/chromium/src/stellite'
  container_build_script_path = os.path.join(container_stellite_path,
                                             'docker', 'bin')
  build_command = [
    'docker', 'run',
    '--rm',
    '-v', '{}:{}'.format(stellite_path(), container_stellite_path),
    '-v', '{}:{}'.format(output_path, '/out'),
    '-e', '{}={}'.format('OUT', '/out'),
    '-e', '{}={}'.format('CHROMIUM_PATH', '/root/chromium'),
    '-e', '{}={}'.format('TARGET', target),
    '-e', '{}={}'.format('ACTION', 'build'),
    '-e', '{}={}'.format('TARGET_TYPE', target_type),
    '-t', build_image,
    os.path.join(container_build_script_path, 'build_android.sh'),
  ]

  execute(build_command, env=docker_env)


def linux_android_build(target, output_path, chromium_path=None,
                        target_type=STATIC_LIBRARY):
  """Android target_platform build"""
  if platform.linux_distribution()[0].lower() != 'ubuntu':
    docker_android_build(target, output_path, chromium_path=chromium_path,
                         target_type=target_type)
    return

  if not (target == STELLITE):
    raise Exception('unsupported android build target: {}'.format(target))

  if not chromium_path:
    chromium_path = fetch_target_chromium(ANDROID)
  chromium_path = os.path.normpath(os.path.abspath(chromium_path))

  gyp_defines = {
    'OS': 'android',
    'component': 'static_library',
    'fastbuild': 2,
    'disable_ftp_support': 1,
  }
  build_base_path = os.path.join(chromium_path, 'src')

  for target_arch, target_info in ANDROID_TARGETS.iteritems():
    platform_gyp_defines = copy.deepcopy(gyp_defines)
    platform_gyp_defines.update(target_info)

    platform_build_path = os.path.join(build_base_path,
                                       'out_android_{}'.format(target_arch))
    if not os.path.exists(platform_build_path):
      patch_diff(chromium_path, 'boringssl_noasm.diff')
      generate_build_script(chromium_path, platform_gyp_defines,
                            platform_build_path)

    platform_release_build_path = os.path.join(platform_build_path, 'Release')
    ninja_build(ninja_target_name(target), platform_release_build_path)

    lib_path = android_link_library(target_arch, chromium_path,
                                    platform_release_build_path,
                                    ANDROID_EXCLUDE_OBJECTS, STELLITE,
                                    target_type=target_type)

    platform_output_path = os.path.join(output_path,
                                        'android_{}'.format(target_arch))
    if os.path.exists(platform_output_path):
      remove_path(platform_output_path)
    make_directory(platform_output_path)

    copy_file(lib_path, platform_output_path)

  # Copy header files
  common_output_path = os.path.join(output_path, 'android_common')
  if os.path.exists(common_output_path):
    remove_path(common_output_path)

  header_output_path = os.path.join(common_output_path, 'include')
  make_directory(header_output_path)
  copy_include_file(header_output_path)

  # Copy lib.java/*.jar files
  javalib_output_path = os.path.join(common_output_path, 'lib.java')
  if not os.path.exists(javalib_output_path):
    make_directory(javalib_output_path)

  javalib_build_path = os.path.join(build_base_path, 'out_android_armv6',
                                    'Release', 'lib.java')
  for jar_file in fnmatch.filter(os.listdir(javalib_build_path), '*.jar'):
    if 'dex' in jar_file:
      continue
    copy_file(os.path.join(javalib_build_path, jar_file), javalib_output_path)


def linux_stellite_build(output_path, chromium_path=None):
  """build a stellite on linux"""
  if not chromium_path:
    chromium_path = fetch_target_chromium(LINUX)
  chromium_path = os.path.normpath(os.path.abspath(chromium_path))

  gyp_defines = {
    'clang': 1,
    'disable_ftp_support': 1,
    'fastbuild': 2,
  }
  build_base_path = os.path.join(chromium_path, 'src')

  platform_build_path = os.path.join(build_base_path, 'out_linux')
  if not os.path.exists(platform_build_path):
    generate_build_script(chromium_path, gyp_defines, platform_build_path)

  platform_output_path = os.path.join(output_path, 'linux')
  if os.path.exists(platform_output_path):
    remove_path(platform_output_path)
  make_directory(platform_output_path)

  platform_release_build_path = os.path.join(platform_build_path, 'Release')
  ninja_build(ninja_target_name(STELLITE), platform_release_build_path)
  lib_path = linux_link_static_library(platform_release_build_path,
                                       LINUX_EXCLUDE_OBJECTS, STELLITE)
  copy_file(lib_path, platform_output_path)

  copy_include_file(platform_output_path)


def linux_quic_server_build(output_path, chromium_path=None):
  """"""
  if not chromium_path:
    chromium_path = fetch_target_chromium(LINUX)
  chromium_path = os.path.normpath(os.path.abspath(chromium_path))

  gyp_defines = {
    'clang': 1,
    'disable_ftp_support': 1,
    'fastbuild': 2,
  }
  build_base_path = os.path.join(chromium_path, 'src')

  platform_build_path = os.path.join(build_base_path, 'out_linux')
  if not os.path.exists(platform_build_path):
    generate_build_script(chromium_path, gyp_defines, platform_build_path)

  platform_output_path = os.path.join(output_path, 'linux')
  if os.path.exists(platform_output_path):
    remove_path(platform_output_path)
  make_directory(platform_output_path)

  platform_release_build_path = os.path.join(platform_build_path, 'Release')
  ninja_build(QUIC_THREAD_SERVER, platform_release_build_path)
  copy_file(os.path.join(platform_release_build_path, QUIC_THREAD_SERVER),
            platform_output_path)


def linux_client_binder_build(output_path, chromium_path=None):
  """build client stub"""
  if not chromium_path:
    chromium_path = fetch_target_chromium(LINUX)
  chromium_path = os.path.normpath(os.path.abspath(chromium_path))

  gyp_defines = {
    'clang': 1,
    'disable_ftp_support': 1,
    'fastbuild': 2,
  }
  build_base_path = os.path.join(chromium_path, 'src')

  platform_build_path = os.path.join(build_base_path, 'out_linux')
  if not os.path.exists(platform_build_path):
    generate_build_script(chromium_path, gyp_defines, platform_build_path)

  platform_output_path = os.path.join(output_path, 'linux')
  if os.path.exists(platform_output_path):
    remove_path(platform_output_path)
  make_directory(platform_output_path)

  platform_release_build_path = os.path.join(platform_build_path, 'Release')
  ninja_build(ninja_target_name(CLIENT_BINDER), platform_release_build_path)
  lib_path = os.path.join(platform_release_build_path, 'lib',
                          'libstellite_client_binder.so')
  copy_file(lib_path, platform_output_path)


def linux_build(target, target_platform, output_path, chromium_path=None,
                target_type=STATIC_LIBRARY):
  """"""
  if target_platform == ANDROID:
    linux_android_build(target, output_path, chromium_path=chromium_path,
                        target_type=target_type)
    return

  if target_platform != LINUX:
    raise Exception('target platform error: {}'.format(target_platform))

  if target == STELLITE:
    linux_stellite_build(output_path, chromium_path)
    return

  if target == QUIC_SERVER:
    linux_quic_server_build(output_path, chromium_path)
    return

  if target == CLIENT_BINDER:
    linux_client_binder_build(output_path, chromium_path)
    return

  raise Exception('unknown target error: {}'.format(target))


def build(arguments):
  """Build entry point"""
  target = arguments.target
  if not target in (STELLITE, QUIC_SERVER, CLIENT_BINDER):
    raise Exception('unknown --target error')

  chromium_path = arguments.chromium_path
  if chromium_path:
    if not is_chromium_path(chromium_path):
      raise Exception('--chromium-path is not exist error')
    chromium_path = os.path.normpath(os.path.abspath(chromium_path))

  target_platform = arguments.target_platform
  target_type = arguments.target_type
  output_path = arguments.out

  local_platform = platform.system().lower()
  if local_platform == DARWIN:
    darwin_build(target, target_platform, output_path,
                 chromium_path=chromium_path, target_type=target_type)
    return

  if local_platform == LINUX:
    linux_build(target, target_platform, output_path,
                chromium_path=chromium_path, target_type=target_type)
    return

  raise Exception('unknown local platfrom error')


def clean(target_platform, chromium_path=None):
  """Clean generated build script"""
  chromium_path = chromium_path or target_chromium_path(target_platform)
  chromium_path = os.path.normpath(os.path.abspath(chromium_path))
  if not os.path.exists(chromium_path):
    return

  if not is_chromium_path(chromium_path):
    raise Exception('invalid chromium path: {}'.format(chromium_path))

  src_path = os.path.join(chromium_path, 'src')
  for path in filter(lambda x: x.startswith('out'), os.listdir(src_path)):
    remove_path(os.path.join(chromium_path, 'src', path))

  sync_chromium_tag(chromium_path)


def process(arguments):
  """process action"""
  if arguments.cache_dir:
    set_third_party_path(arguments.cache_dir)

  fetch_depot_tools()

  action = arguments.action

  if action == CLEAN:
    clean(arguments.target_platform, arguments.chromium_path)
    return

  if action == BUILD:
    build(arguments)
    return

  if action == CLEAN_BUILD:
    clean(arguments.target_platform, arguments.chromium_path)
    build(arguments)
    return

  raise Exception('unknown action error: {}'.format(action))


def main(args):
  """Main entry point"""
  parser = argparse.ArgumentParser()
  parser.add_argument('--chromium-path', help='specify a chromium path')
  parser.add_argument('--out', help='build output path',
                      default=os.path.join(stellite_path(), 'out'))
  parser.add_argument('--target-platform', help='ios, android, darwin, linux',
                      choices=[IOS, ANDROID, DARWIN, LINUX], required=True)
  parser.add_argument('--target', help='stellite library or quic server',
                      choices=[STELLITE, QUIC_SERVER, CLIENT_BINDER],
                      required=True)
  parser.add_argument('--target-type', help='library type',
                      choices=[STATIC_LIBRARY, SHARED_LIBRARY],
                      default=STATIC_LIBRARY)
  parser.add_argument('--cache-dir', default=None)

  parser.add_argument('action', help='build action',
                      choices=[CLEAN, BUILD, CLEAN_BUILD, UNITTEST,
                               SYNC_CHROMIUM])

  arguments = parser.parse_args(args)
  process(arguments)



if __name__ == '__main__':
  main(sys.argv[1:])
