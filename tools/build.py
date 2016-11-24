#!/usr/bin/env python
#
# write by @snibug

import argparse
import distutils.spawn
import multiprocessing
import os
import fnmatch
import platform
import shutil
import subprocess
import sys
import pipes

ALL = 'all'
ANDROID = 'android'
BUILD = 'build'
CHROMIUM = 'chromium'
CLEAN = 'clean'
CLIENT_BINDER = 'client_binder'
CONFIGURE = 'configure'
DARWIN = 'darwin'
IOS = 'ios'
LINUX = 'linux'
MAC = 'mac'
QUIC_CLIENT = 'quic_client'
SHARED_LIBRARY = 'shared_library'
STATIC_LIBRARY = 'static_library'
STELLITE_HTTP_CLIENT = 'stellite_http_client'
STELLITE_QUIC_SERVER = 'stellite_quic_server'
TARGET = 'target'
TRIDENT_HTTP_CLIENT = 'trident_http_client'
UBUNTU = 'ubuntu'
WINDOWS = 'windows'

GIT_DEPOT = 'https://chromium.googlesource.com/chromium/tools/depot_tools.git'

GCLIENT_IOS = """
solutions = [
  {
    "managed": False,
    "name": "src",
    "url": "https://chromium.googlesource.com/chromium/src.git",
    "custom_deps": {},
    "deps_file": ".DEPS.git",
    "safesync_url": "",
  },
]
target_os = [\"ios\"]
target_os_only = \"True\"
"""

GCLIENT_ANDROID = """
solutions = [
  {
    "managed": False,
    "name": "src",
    "url": "https://chromium.googlesource.com/chromium/src.git",
    "custom_deps": {},
    "deps_file": ".DEPS.git",
    "safesync_url": "",
  },
]
target_os = [\"android\"]
target_os_only = \"True\"
"""

GN_ARGS_LINUX = """
is_component_build = false
disable_file_support = true
disable_ftp_support = true
target_cpu = "x64"
target_os = "linux"
"""

GN_ARGS_MAC = """
is_component_build = false
disable_file_support = true
disable_ftp_support = true
target_cpu = "x64"
target_os = "mac"
"""

GN_ARGS_IOS = """
disable_file_support = true
disable_ftp_support = true
enable_dsyms = false
enable_stripping = enable_dsyms
ios_enable_code_signing = false
is_component_build = false
is_debug = false
is_official_build = false
symbol_level = 1
target_cpu = "{}"
target_os = "ios"
use_xcode_clang = is_official_build
"""

GN_ARGS_ANDROID = """
disable_file_support = true
disable_ftp_support = true
is_clang = true
is_component_build = false
is_debug = false
symbol_level = 1
target_cpu = "{}"
target_os = "android"
"""

GN_ARGS_WINDOWS = """
is_component_build = {}
is_debug = false
symbol_level = 0
target_cpu = "x64"
target_os = "win"
"""

CHROMIUM_DEPENDENCY_DIRECTORIES = [
  'base',
  'build',
  'build_overrides',
  'buildtools',
  'components/url_matcher',
  'crypto',
  'net',
  'sdch',
  'testing',
  'third_party/apple_apsl',
  'third_party/binutils',
  'third_party/boringssl',
  'third_party/brotli',
  'third_party/ced',
  'third_party/closure_compiler',
  'third_party/drmemory',
  'third_party/icu',
  'third_party/instrumented_libraries',
  'third_party/libxml/',
  'third_party/llvm-build',
  'third_party/modp_b64',
  'third_party/protobuf',
  'third_party/pyftpdlib',
  'third_party/pywebsocket',
  'third_party/re2',
  'third_party/tcmalloc',
  'third_party/tlslite',
  'third_party/yasm',
  'third_party/zlib',
  'tools',
  'url',
]

ANDROID_DEPENDENCY_DIRECTORIES = [
  'base',
  'build',
  'build_overrides',
  'buildtools',
  'components/url_matcher',
  'crypto',
  'net',
  'sdch',
  'testing',
  'third_party/apple_apsl',
  'third_party/binutils',
  'third_party/boringssl',
  'third_party/brotli',
  'third_party/ced',
  'third_party/closure_compiler',
  'third_party/drmemory',
  'third_party/icu',
  'third_party/instrumented_libraries',
  'third_party/libxml/',
  'third_party/llvm-build',
  'third_party/modp_b64',
  'third_party/protobuf',
  'third_party/pyftpdlib',
  'third_party/pywebsocket',
  'third_party/re2',
  'third_party/tcmalloc',
  'third_party/tlslite',
  'third_party/yasm',
  'third_party/zlib',
  'tools',
  'url',
  'v8',
  'third_party/WebKit',
  'third_party/accessibility_test_framework',
  'third_party/android_async_task',
  'third_party/android_crazy_linker',
  'third_party/android_data_chart',
  'third_party/android_media',
  'third_party/android_opengl',
  'third_party/android_platform',
  'third_party/android_protobuf',
  'third_party/android_support_test_runner',
  'third_party/android_swipe_refresh',
  'third_party/android_tools',
  'third_party/apache_velocity',
  'third_party/ashmem',
  'third_party/bouncycastle',
  'third_party/byte_buddy',
  'third_party/catapult',
  'third_party/guava',
  'third_party/hamcrest',
  'third_party/icu4j',
  'third_party/ijar',
  'third_party/intellij',
  'third_party/jsr-305',
  'third_party/junit',
  'third_party/mockito',
  'third_party/objenesis',
  'third_party/ow2_asm',
  'third_party/robolectric',
  'third_party/sqlite4java',
]

WINDOWS_DEPENDENCY_DIRECTORIES= [
  'base',
  'build',
  'build_overrides',
  'buildtools',
  'components/url_matcher',
  'crypto',
  'net',
  'sdch',
  'testing',
  'third_party/llvm-build',
  'third_party/apple_apsl',
  'third_party/binutils',
  'third_party/boringssl',
  'third_party/brotli',
  'third_party/ced',
  'third_party/closure_compiler',
  'third_party/drmemory',
  'third_party/icu',
  'third_party/instrumented_libraries',
  'third_party/libxml/',
  'third_party/modp_b64',
  'third_party/protobuf',
  'third_party/pyftpdlib',
  'third_party/pywebsocket',
  'third_party/re2',
  'third_party/tcmalloc',
  'third_party/tlslite',
  'third_party/yasm',
  'third_party/zlib',
  'tools',
  'url',
]

MAC_EXCLUDE_OBJECTS = [
#  'libprotobuf_full_do_not_use.a',
#  'libprotoc_lib.a',
  'libprotobuf_full.a',
]

IOS_EXCLUDE_OBJECTS = [
#  'libprotobuf_full_do_not_use.a',
#  'libprotoc_lib.a',
  'libprotobuf_full.a',
]

ANDROID_EXCLUDE_OBJECTS = [
  #'cpu_features.cpu-features.o',
]

LINUX_EXCLUDE_OBJECTS = [
  'libprotobuf_full.a',
]

def detect_host_platform():
  """detect host architecture"""
  host_platform_name = platform.system().lower()
  if host_platform_name == DARWIN:
    return MAC
  return host_platform_name


def detect_host_os():
  """detect host operating system"""
  return platform.system().lower()


def detect_host_arch():
  return platform.uname()[4]


def option_parser(args):
  """fetching tools arguments parser"""
  parser = argparse.ArgumentParser()

  host_platform = detect_host_platform()
  parser.add_argument('--target-platform',
                      choices=[LINUX, ANDROID, IOS, MAC, WINDOWS],
                      help='default platform {}'.format(host_platform),
                      default=host_platform)

  parser.add_argument('--target',
                      choices=[STELLITE_QUIC_SERVER, STELLITE_HTTP_CLIENT,
                               TRIDENT_HTTP_CLIENT, CLIENT_BINDER],
                      default=TRIDENT_HTTP_CLIENT)

  parser.add_argument('--target-type',
                      choices=[STATIC_LIBRARY, SHARED_LIBRARY],
                      default=STATIC_LIBRARY)

  parser.add_argument('-v', '--verbose', action='store_true')

  parser.add_argument('action', choices=[CLEAN, BUILD], default=BUILD)

  return parser.parse_args(args)


def build_object(options):
  if options.target_platform == ANDROID:
    return AndroidBuild(options.target, options.target_type)

  if options.target_platform == MAC:
    return MacBuild(options.target, options.target_type)

  if options.target_platform == IOS:
    return IOSBuild(options.target, options.target_type)

  if options.target_platform == LINUX:
    return LinuxBuild(options.target, options.target_type)

  if options.target_platform == WINDOWS:
    return WindowsBuild(options.target, options.target_type)

  raise Exception('unsupported target_platform: {}'.format(options.target_type))


def which_application(name):
  return distutils.spawn.find_executable(name)


def copy_tree(src, dest):
  """recursivly copy a directory from |src| to |dest|"""
  if not os.path.exists(dest):
    os.makedirs(dest)

  for dir_member in os.listdir(src):
    src_name = os.path.join(src, dir_member)
    dest_name = os.path.join(dest, dir_member)
    if os.path.isdir(src_name):
      copy_tree(src_name, dest_name)
      continue
    shutil.copy2(src_name, dest_name)


class BuildObject(object):
  """build stellite client and server"""

  def __init__(self, target, target_type, target_platform, verbose=False):
    self._target = target
    self._target_type = target_type
    self._target_platform = target_platform
    self._verbose = verbose

    self.fetch_depot_tools()

  @property
  def verbose(self):
    return self._verbose

  @property
  def target(self):
    return self._target

  @property
  def target_type(self):
    return self._target_type

  @property
  def target_platform(self):
    return self._target_platform

  @property
  def root_path(self):
    """reprotobuf_full/turn root absolute path"""
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

  @property
  def output_path(self):
    return os.path.join(self.root_path, 'output')

  @property
  def third_party_path(self):
    """return third_party path"""
    return os.path.join(self.root_path, 'third_party')

  @property
  def depot_tools_path(self):
    return os.path.join(self.third_party_path, 'depot_tools')

  @property
  def gclient_path(self):
    """return gclient path"""
    return os.path.join(self.depot_tools_path, 'gclient')

  @property
  def buildspace_path(self):
    """return source code directory for build.

    it was getter from chromium essential code and code"""
    return os.path.join(self.root_path, 'build_{}'.format(self.target_platform))

  @property
  def buildspace_src_path(self):
    return os.path.join(self.buildspace_path, 'src')

  @property
  def gn_path(self):
    """return gn path"""
    return os.path.join(self.depot_tools_path, 'gn')

  @property
  def build_output_path(self):
    """return object file path that store a compiled files"""
    out_dir = 'out_{}'.format(self.target_platform)
    return os.path.join(self.buildspace_src_path, out_dir)

  @property
  def stellite_path(self):
    """return stellite code path"""
    return os.path.join(self.root_path, 'stellite')

  @property
  def trident_path(self):
    """return trident_stellite code path"""
    return os.path.join(self.root_path, 'trident')

  @property
  def modified_files_path(self):
    """return modified_files path"""
    return os.path.join(self.root_path, 'modified_files')

  @property
  def chromium_path(self):
    chromium_dir = 'chromium_{}'.format(self.target_platform)
    return os.path.join(self.third_party_path, chromium_dir)

  @property
  def chromium_src_path(self):
    return os.path.join(self.chromium_path, 'src')

  @property
  def chromium_branch(self):
    return self.read_chromium_branch()

  @property
  def chromium_tag(self):
    return self.read_chromium_tag()

  @property
  def clang_compiler_path(self):
    return os.path.join(self.chromium_src_path, 'third_party', 'llvm-build',
                        'Release+Asserts', 'bin', 'clang++')

  @property
  def iphone_sdk_path(self):
    return self.xcode_sdk_path('iphoneos')

  @property
  def simulator_sdk_path(self):
    return self.xcode_sdk_path('iphonesimulator')

  @property
  def mac_sdk_path(self):
    return self.xcode_sdk_path('macosx')

  @property
  def dependency_directories(self):
    return CHROMIUM_DEPENDENCY_DIRECTORIES

  @property
  def stellite_http_client_header_files(self):
    include_path = os.path.join(self.stellite_path, 'include')
    return map(lambda x: os.path.join(include_path, x),
               os.listdir(include_path))

  @property
  def trident_http_client_header_files(self):
    include_path = os.path.join(self.trident_path, 'include')
    return map(lambda x: os.path.join(include_path, x),
               os.listdir(include_path))

  def xcode_sdk_path(self, osx_target):
    """return xcode sdk path"""
    command = ['xcrun', '-sdk', osx_target, '--show-sdk-path']
    job = subprocess.Popen(command, stdout=subprocess.PIPE)
    job.wait()
    return job.stdout.read().strip()

  def execute_with_error(self, command, env=None, cwd=None):
    env = env or os.environ.copy()
    print('Running: %s' % (' '.join(pipes.quote(x) for x in command)))

    job = subprocess.Popen(command, env=env, cwd=cwd)
    return job.wait() == 0


  def execute(self, command, env=None, cwd=None):
    """execute shell command"""
    res = self.execute_with_error(command, env=env, cwd=cwd)
    if bool(res) == True:
      return

    raise Exception('command execution are failed')

  def fetch_depot_tools(self):
    """get depot_tools code"""
    if os.path.exists(self.depot_tools_path):
      return

    os.makedirs(self.depot_tools_path)
    self.execute(['git', 'clone', GIT_DEPOT, self.depot_tools_path],
                 cwd=self.depot_tools_path)

  def fetch_chromium(self):
    """fetch chromium"""
    if os.path.exists(self.chromium_path):
      print('chromium is already exists: {}'.format(self.chromium_path))
      return False

    os.makedirs(self.chromium_path)

    fetch_path = os.path.join(self.depot_tools_path, 'fetch')
    fetch_command = [fetch_path, '--nohooks', 'chromium']
    self.execute(fetch_command, cwd=self.chromium_path)

    return self.fetch_toolchain()

  def fetch_toolchain(self):
    """fetch build toolchain"""
    return True

  def remove_chromium(self):
    """remove chromium code"""
    if not os.path.exists(self.chromium_path):
      return

    print('remove chromium code: '.format(self.chromium_path))
    shutil.rmtree(self.chromium_path)

  def generate_ninja_script(self, gn_args, gn_options=None):
    """generate ninja build script using gn."""
    gn_options = gn_options or []

    if not os.path.exists(self.build_output_path):
      os.makedirs(self.build_output_path)

    gn_args_file_path = os.path.join(self.build_output_path, 'args.gn')
    with open(gn_args_file_path, 'w') as gn_args_file:
      gn_args_file.write(gn_args)

    print('generate ninja script ...')

    command = [self.gn_path]
    if len(gn_options) > 0:
      command.extend(gn_options)
    command.append('gen')
    command.append(self.build_output_path)

    self.execute(command, cwd=self.buildspace_src_path)

  def write_gclient(self, context):
    gclient_info_path = os.path.join(self.chromium_path, '.gclient')
    if os.path.exists(gclient_info_path):
      os.remove(gclient_info_path)

    with open(gclient_info_path, 'a') as gclient_meta_file:
      gclient_meta_file.write(context)

  def check_target_platform(self):
    """check host-platform to build a targe or not"""
    host_platform = detect_host_platform()
    if self.target_platform in (IOS, MAC):
      if not host_platform == MAC:
        return False

    if self.target_platform == LINUX:
      if host_platform != LINUX:
        return False

    if self.target_platform == ANDROID:
      if not UBUNTU in platform.uname()[3].lower():
        return False

    return True

  def check_chromium_repository(self):
    """check either chromium are valid or not"""
    if not os.path.exists(self.chromium_path):
      return False

    if not os.path.exists(self.chromium_src_path):
      return False

    if not os.path.isdir(self.chromium_src_path):
      return False

    if not os.path.exists(os.path.join(self.chromium_path, '.gclient')):
      return False

    if not os.path.exists(os.path.join(self.chromium_path, 'src', 'DEPS')):
      return False

    return True

  def read_chromium_branch(self):
    """read chromium git branch"""
    command = ['git', 'rev-parse', '--abbrev-ref', 'HEAD']

    try:
      job = subprocess.Popen(command, cwd=self.chromium_src_path,
                             stdout=subprocess.PIPE)
      job.wait()
      return job.stdout.read().strip()
    except Exception:
      return None

  def read_chromium_tag(self):
    """read tag string that write on file """
    chromium_tag_file_path = os.path.join(self.root_path, 'chromium.tag')
    if not os.path.exists(chromium_tag_file_path):
      raise Exception('chromium.tag is not exist error')

    with open(chromium_tag_file_path, 'r') as tag:
      return tag.read().strip()

  def synchronize_chromium_tag(self):
    """sync chromium"""
    print('check chromium ...')
    if not self.check_chromium_repository():
      raise Exception('invalid chromium repository error')

    print('check chromium tag ...')
    if self.chromium_tag in self.chromium_branch:
      return True

    cwd = self.chromium_src_path

    self.execute(['git', 'fetch', '--tags'], cwd=cwd)
    self.execute(['git', 'reset', '--hard'], cwd=cwd)

    branch = 'chromium_{}'.format(self.chromium_tag)
    if not self.execute_with_error(['git', 'checkout', branch], cwd=cwd):
      make_branch_command = ['git', 'checkout', '-b', branch, self.chromium_tag]
      self.execute(make_branch_command, cwd=cwd)

    command = [self.gclient_path, 'sync', '--with_branch_heads',
               '--jobs', str(multiprocessing.cpu_count() * 4)]
    self.execute(command, cwd=cwd)

  def synchronize_buildspace(self):
    """synchronize code for building libchromium.a"""
    if not os.path.exists(self.buildspace_src_path):
      print('make {} ...'.format(self.buildspace_src_path))
      os.makedirs(self.buildspace_src_path)

    for target_dir in self.dependency_directories:
      if os.path.exists(os.path.join(self.buildspace_src_path, target_dir)):
        continue
      print('copy chromium {} ...'.format(target_dir))
      copy_tree(os.path.join(self.chromium_src_path, target_dir),
                os.path.join(self.buildspace_src_path, target_dir))

    print('copy .gclient ...')
    shutil.copy(os.path.join(self.chromium_path, '.gclient'),
                self.buildspace_path)

    print('copy modified_files ...')
    copy_tree(self.modified_files_path, self.buildspace_src_path)

    print('copy chromium/src/.gn ...')
    shutil.copy(os.path.join(self.chromium_src_path, '.gn'),
                self.buildspace_src_path)

    print('copy stellite files...')
    self.copy_stellite_code()

  def copy_stellite_code(self):
    """copy stellite code to buildspace"""
    stellite_buildspace_path = os.path.join(self.buildspace_src_path,
                                            'stellite')
    if os.path.exists(stellite_buildspace_path):
      os.remove(stellite_buildspace_path)

    command = ['ln', '-s', self.stellite_path]
    self.execute_with_error(command, cwd=self.buildspace_src_path)

    trident_buildspace_path = os.path.join(self.buildspace_src_path,
                                           'trident')
    if os.path.exists(trident_buildspace_path):
      os.remove(trident_buildspace_path)

    command = ['ln', '-s', self.trident_path]
    self.execute_with_error(command, cwd=self.buildspace_src_path)

  def build_target(self):
    command = ['ninja']
    if self.verbose:
      command.append('-v')
    command.extend(['-C', self.build_output_path, self.target])
    self.execute(command)

  def check_depot_tools_or_install(self):
    if os.path.exists(self.depot_tools_path):
      return
    print('fetch depot_tools ...')
    self.fetch_depot_tools()

  def gclient_sync(self):
    command = [self.gclient_path, 'sync',
               '--jobs', str(multiprocessing.cpu_count() * 4)]
    self.execute(command, cwd=self.chromium_path)

  def pattern_files(self, root, pattern, exclude_patterns=None):
    """return pattern file path list"""
    res = []
    exclude_patterns = exclude_patterns or []
    for path, _, file_list in os.walk(root):
      for matched in fnmatch.filter(file_list, pattern):
        is_exclude = False
        for exclude_filename in exclude_patterns:
          if matched in exclude_filename:
            is_exclude = True
            break
        if is_exclude:
          continue
        res.append(os.path.join(path, matched))
    return res

  def build(self):
    """build target"""
    raise NotImplementedError()

  def clean(self):
    """clean buildspace"""
    if not os.path.exists(self.build_output_path):
      return
    shutil.rmtree(self.build_output_path)


class AndroidBuild(BuildObject):
  """android build"""
  def __init__(self, target, target_type, verbose=False, target_arch=None):
    self._target_arch = target_arch or ALL
    super(self.__class__, self).__init__(target, target_type, ANDROID,
                                         verbose=verbose)

  @property
  def target_arch(self):
    return self._target_arch

  @property
  def build_output_path(self):
    out_dir = 'out_{}_{}'.format(self.target_platform, self.target_arch)
    return os.path.join(self.buildspace_path, 'src', out_dir, 'obj')

  @property
  def ndk_root_path(self):
    return os.path.join(self.chromium_path, 'src', 'third_party',
                        'android_tools', 'ndk')

  @property
  def android_app_abi(self):
    if self.target_arch == 'armv6':
      return 'armeabi'

    if self.target_arch == 'armv7':
      return 'armeabi-v7a'

    if self.target_arch == 'arm64':
      return 'arm64-v8a'

    if self.target_arch == 'x86':
      return 'x86'

    if self.target_arch == 'x64':
      return 'x86_64'

    raise Exception('unknown target architecture: {}'.format(self.target_arch))

  @property
  def android_toolchain_relative(self):
    if self.target_arch in ('armv6', 'armv7'):
      return 'arm-linux-androideabi-4.9'

    if self.target_arch == 'arm64':
      return 'aarch64-linux-android-4.9'

    if self.target_arch == 'x86':
      return 'x86-4.9'

    if self.target_arch == 'x64':
      return 'x86_64-4.9'

    raise Exception('unknown target architecture: {}'.format(self.target_arch))

  @property
  def android_libcpp_root(self):
    return os.path.join(self.ndk_root_path, 'sources', 'cxx-stl', 'llvm-libc++')

  @property
  def android_libcpp_libs_dir(self):
    return os.path.join(self.android_libcpp_root, 'libs', self.android_app_abi)

  @property
  def android_ndk_sysroot(self):
    ndk_platforms_path = os.path.join(self.ndk_root_path, 'platforms')
    if self.target_arch in ('armv6', 'armv7'):
      return os.path.join(ndk_platforms_path, 'android-16', 'arch-arm')

    if self.target_arch == 'arm64':
      return os.path.join(ndk_platforms_path, 'android-21', 'arch-arm64')

    if self.target_arch == 'x86':
      return os.path.join(ndk_platforms_path, 'android-16', 'arch-x86')

    if self.target_arch == 'x64':
      return os.path.join(ndk_platforms_path, 'android-21', 'arch-x86_64')

    raise Exception('unknown target error: {}'.format(self.target_arch))

  @property
  def android_ndk_lib_dir(self):
    if self.target_arch in ('armv6', 'armv7', 'x86', 'x64'):
      return os.path.join('usr', 'lib')

    if self.target_arch == 'x64':
      return os.path.join('usr', 'lib64')

    raise Exception('unknown target architecture: {}'.format(self.target_arch))

  @property
  def android_ndk_lib(self):
    return os.path.join(self.android_ndk_sysroot, self.android_ndk_lib_dir)

  @property
  def android_toolchain_path(self):
    return os.path.join(self.ndk_root_path, 'toolchains',
                        self.android_toolchain_relative, 'prebuilt',
                        '{}-{}'.format(detect_host_os(), detect_host_arch()))

  @property
  def android_compiler_path(self):
    toolchain_path = os.path.join(self.android_toolchain_path, 'bin')
    filtered = filter(lambda x: x.endswith('g++'), os.listdir(toolchain_path))
    return os.path.join(toolchain_path, filtered[0])

  @property
  def android_ar_path(self):
    toolchain_path = os.path.join(self.android_toolchain_path, 'bin')
    filtered = filter(lambda x: x.endswith('ar'), os.listdir(toolchain_path))
    return os.path.join(toolchain_path, filtered[0])

  @property
  def binutils_path(self):
    return os.path.join(self.chromium_path, 'src', 'third_party', 'binutils',
                        'Linux_x64', 'Release', 'bin')

  @property
  def android_libgcc_filename(self):
    toolchain_path = os.path.join(self.android_toolchain_path, 'bin')
    filtered = filter(lambda x: x.endswith('gcc'), os.listdir(toolchain_path))
    gcc_path = os.path.join(toolchain_path, filtered[0])
    job = subprocess.Popen([gcc_path, '-print-libgcc-file-name'],
                           stdout=subprocess.PIPE)
    job.wait()
    return job.stdout.read().strip()

  @property
  def android_abi_target(self):
    if self.target_arch in ('armv6', 'armv7'):
      return 'arm-linux-androideabi'
    if self.target_arch == 'x86':
      return 'i686-linux-androideabi'
    if self.target_arch == 'arm64':
      return 'aarch64-linux-android'
    if self.target_arch == 'x64':
      return 'x86_64-linux-androideabi'
    raise Exception('unknown target architecture error')

  @property
  def dependency_directories(self):
    return ANDROID_DEPENDENCY_DIRECTORIES

  @property
  def chromium_java_lib_deps(self):
    # chromium android java library are same for different cpu architecture
    java_lib_path = os.path.join(self.buildspace_src_path, 'out_android_armv6',
                                 'obj', 'lib.java')
    return self.pattern_files(java_lib_path, '*.jar')

  def synchronize_buildspace(self):
    super(self.__class__, self).synchronize_buildspace()
    buildspace_src = os.path.join(self.buildspace_path, 'src')
    for target_dir in ANDROID_DEPENDENCY_DIRECTORIES:
      if os.path.exists(os.path.join(buildspace_src, target_dir)):
        continue
      print('copy chromium {} ...'.format(target_dir))
      copy_tree(os.path.join(self.chromium_src_path, target_dir),
                os.path.join(buildspace_src, target_dir))

  def clang_arch(self, arch):
    if arch in ('x86', 'x64', 'arm64'):
      return arch
    if arch in ('armv6', 'armv7'):
      return 'arm'
    raise Exception('undefined android clang arch')

  def appendix_gn_args(self, arch):
    if arch == 'armv6':
      return 'arm_version = 6'
    if arch == 'armv7':
      return 'arm_version = 7'
    return ''

  def package_target(self):
    if self.target_type == STATIC_LIBRARY:
      return self.link_static_library()

    if self.target_type == SHARED_LIBRARY:
      return self.link_shared_library()

    raise Exception('invalid package target')

  def fetch_toolchain(self):
    self.write_gclient(GCLIENT_ANDROID)
    self.gclient_sync()
    return True

  def link_static_library(self):
    library_name = 'lib{}_{}_{}.a'.format(self.target, self.target_platform,
                                          self.target_arch)
    library_path = os.path.join(self.build_output_path, library_name)
    command = [
      self.android_ar_path,
      'rsc', os.path.join(self.build_output_path, library_name),
    ]

    for filename in self.pattern_files(self.build_output_path, '*.o'):
      command.append(filename)
    self.execute(command)
    return library_path

  def link_shared_library(self):
    library_name = 'lib{}_{}_{}.so'.format(self.target, self.target_platform,
                                           self.target_arch)
    command = [
      self.clang_compiler_path,
      '-Wl,-shared',
      '-Wl,--fatal-warnings',
      '-fPIC',
      '-Wl,-z,noexecstack',
      '-Wl,-z,now',
      '-Wl,-z,relro',
      '-Wl,-z,defs',
      '-Wl,--as-needed',
      '--gcc-toolchain={}'.format(self.android_toolchain_path),
      '-fuse-ld=gold',
      '-Wl,--icf=all',
      '-Wl,--build-id=sha1',
      '-Wl,--no-undefined',
      '-Wl,--exclude-libs=libgcc.a',
      '-Wl,--exclude-libs=libc++_static.a',
      '-Wl,--exclude-libs=libvpx_assembly_arm.a',
      '--target={}'.format(self.android_abi_target),
      '-nostdlib',
      '-Wl,--warn-shared-textrel',
      '--sysroot={}'.format(self.android_ndk_sysroot),
      '-Wl,--warn-shared-textrel',
      '-Wl,-O1',
      '-Wl,--gc-sections',
      '-Wl,-wrap,calloc',
      '-Wl,-wrap,free',
      '-Wl,-wrap,malloc',
      '-Wl,-wrap,memalign',
      '-Wl,-wrap,posix_memalign',
      '-Wl,-wrap,pvalloc',
      '-Wl,-wrap,realloc',
      '-Wl,-wrap,valloc',
      '-Wl,--gc-sections',
    ]

    objs = self.pattern_files(os.path.join(self.build_output_path, 'obj'),
                              '*.o', ANDROID_EXCLUDE_OBJECTS)
    command.extend(objs)

    command.extend([
      '-L{}'.format(self.android_libcpp_libs_dir),
      '-lc++_shared',
      '-lc++abi',
      '-landroid_support',
      self.android_libgcc_filename,
      '-lc',
      '-ldl',
      '-lm',
      '-llog'
    ])

    # armv6, armv7 arch leck of stack trace symbol in stl
    if self.target_arch in ('armv6', 'armv7'):
      command.append('-lunwind')

    library_path = os.path.join(self.build_output_path, library_name)
    command.extend(['-o', library_path])

    self.execute(command)

    return library_path

  def build(self):
    output_list = []
    for arch in ('armv6', 'armv7', 'arm64', 'x86', 'x64'):
      gn_context = GN_ARGS_ANDROID.format(self.clang_arch(arch))
      gn_context += '\n' + self.appendix_gn_args(arch)

      build = AndroidBuild(self.target, self.target_type, target_arch=arch)
      build.generate_ninja_script(gn_args=gn_context)
      build.build_target()
      output_list.append(build.package_target())

    if self.target == STELLITE_HTTP_CLIENT:
      output_list.extend(self.stellite_http_client_header_files)

    if self.target == TRIDENT_HTTP_CLIENT:
      output_list.extend(self.trident_http_client_header_files)

    output_list.extend(self.chromium_java_lib_deps)

    return output_list

  def clean(self):
    for arch in ('armv6', 'armv7', 'arm64', 'x86', 'x64'):
      build = AndroidBuild(self.target, self.target_type, target_arch=arch)
      if not os.path.exists(build.build_output_path):
        continue
      shutil.rmtree(build.build_output_path)


class MacBuild(BuildObject):
  """mac build"""
  def __init__(self, target, target_type, verbose=False):
    super(self.__class__, self).__init__(target, target_type, MAC,
                                         verbose=verbose)

  def build(self):
    self.generate_ninja_script(GN_ARGS_MAC)
    self.build_target()

    if self.target == STELLITE_QUIC_SERVER:
      return [os.path.join(self.build_output_path, self.target)]

    output_list = []
    output_list.append(self.package_target())

    if self.target == STELLITE_HTTP_CLIENT:
      output_list.extend(self.stellite_http_client_header_files)

    if self.target == TRIDENT_HTTP_CLIENT:
      output_list.extend(self.trident_http_client_header_files)

    return output_list

  def package_target(self):
    if self.target_type == SHARED_LIBRARY:
      return self.link_shared_library()

    if self.target_type == STATIC_LIBRARY:
      return self.link_static_library()

    raise Exception('undefined target_type error: {}'.format(self.target_type))

  def link_shared_library(self):
    command = [
      self.clang_compiler_path,
      '-shared',
      '-Wl,-search_paths_first',
      '-Wl,-dead_strip',
      '-isysroot', self.mac_sdk_path,
      '-arch', 'x86_64',
    ]

    for filename in self.pattern_files(self.build_output_path, '*.a',
                                       MAC_EXCLUDE_OBJECTS):
      command.append('-Wl,-force_load,{}'.format(filename))

    library_name = 'lib{}_{}.dylib'.format(self.target, self.target_platform)
    library_path = os.path.join(self.build_output_path, library_name)
    command.extend([
      '-o', library_path,
      '-install_name', '@loader_path/{}'.format(library_name),
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
    self.execute(command, cwd=self.build_output_path)
    return library_path

  def link_static_library(self):
    library_name = 'lib{}.a'.format(self.target)
    libtool_path = which_application('libtool')
    if not libtool_path:
      raise Exception('libtool is not exist error')

    command = [ libtool_path, '-static', ]
    for filename in self.pattern_files(self.build_output_path, '*.a',
                                       MAC_EXCLUDE_OBJECTS):
      command.append(os.path.join(self.build_output_path, filename))

    library_path = os.path.join(self.build_output_path, library_name)
    command.extend([ '-o', library_path ])
    self.execute(command)
    return library_path


class IOSBuild(BuildObject):
  """ios build"""
  def __init__(self, target, target_type, target_arch=None, verbose=False):
    self._target_arch = target_arch or ALL
    super(self.__class__, self).__init__(target, target_type, IOS,
                                         verbose=verbose)

  @property
  def target_arch(self):
    return self._target_arch

  @property
  def build_output_path(self):
    out_dir = 'out_{}_{}'.format(self.target_platform, self.target_arch)
    return os.path.join(self.buildspace_path, 'src', out_dir)

  @property
  def clang_target_arch(self):
    if self.target_arch == 'arm':
      return 'armv7'
    if self.target_arch == 'arm64':
      return 'arm64'
    if self.target_arch == 'x86':
      return 'i386'
    if self.target_arch == 'x64':
      return 'x86_64'
    raise Exception('unsupported ios target architecture error')

  def fetch_toolchain(self):
    self.write_gclient(GCLIENT_IOS)
    self.gclient_sync()
    return True

  def build(self):
    lib_list = []
    for arch in ('x86', 'x64', 'arm', 'arm64'):
      build = IOSBuild(self.target, self.target_type, target_arch=arch)
      build.generate_ninja_script(gn_args=GN_ARGS_IOS.format(arch),
                                  gn_options=['--check'])
      build.build_target()
      lib_list.append(build.package_target())

    output_list = []
    output_list.append(self.link_fat_library(lib_list))

    if self.target == STELLITE_HTTP_CLIENT:
      output_list.extend(self.stellite_http_client_header_files)
    if self.target == TRIDENT_HTTP_CLIENT:
      output_list.extend(self.trident_http_client_header_files)

    return output_list

  def clean(self):
    for arch in ('arm', 'arm64', 'x86', 'x64'):
      build = IOSBuild(self.target, self.target_type, target_arch=arch)
      if not os.path.exists(build.build_output_path):
        continue
      shutil.rmtree(build.build_output_path)

  def package_target(self):
    if self.target_type == STATIC_LIBRARY:
      return self.link_static_library()
    else:
      return self.link_shared_library()

  def link_shared_library(self):
    library_name = 'lib{}_{}.dylib'.format(self.target, self.target_arch)

    sdk_path = self.iphone_sdk_path
    if self.target_arch in ('x86', 'x64'):
      sdk_path = self.simulator_sdk_path

    command = [
      self.clang_compiler_path,
      '-shared',
      '-Wl,-search_paths_first',
      '-Wl,-dead_strip',
      '-miphoneos-version-min=9.0',
      '-isysroot', sdk_path,
      '-arch', self.clang_target_arch,
      '-install_name', '@loader_path/{}'.format(library_name),
    ]
    for filename in self.pattern_files(self.build_output_path, '*.a',
                                       IOS_EXCLUDE_OBJECTS):
      command.append('-Wl,-force_load,{}'.format(filename))

    library_path = os.path.join(self.build_output_path, library_name)
    command.extend([
      '-o', library_path,
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
    self.execute(command)
    return library_path

  def link_static_library(self):
    library_name = 'lib{}_{}.a'.format(self.target, self.target_arch)

    libtool_path = which_application('libtool')
    if not libtool_path:
      raise Exception('libtool is not exist error')

    command = [
      libtool_path,
      '-static',
    ]
    for filename in self.pattern_files(self.build_output_path, '*.a',
                                       IOS_EXCLUDE_OBJECTS):
      command.append(os.path.join(self.build_output_path, filename))

    library_path = os.path.join(self.build_output_path, library_name)
    command.extend([
      '-o', library_path
    ])
    self.execute(command)
    return library_path

  def link_fat_library(self, from_list):
    command = ['lipo', '-create']
    command.extend(from_list)
    command.append('-output')

    fat_filename = 'lib{}.dylib'.format(self.target)
    if self.target_type == STATIC_LIBRARY:
      fat_filename = 'lib{}.a'.format(self.target)

    fat_filepath = os.path.join(self.build_output_path, fat_filename)
    command.append(fat_filepath)

    if os.path.exists(self.build_output_path):
      shutil.rmtree(self.build_output_path)
    os.makedirs(self.build_output_path)

    self.execute(command)
    return fat_filepath


class LinuxBuild(BuildObject):
  """linux build"""
  def __init__(self, target, target_type, verbose=False):
    super(self.__class__, self).__init__(target, target_type, LINUX,
                                         verbose=verbose)

  def build(self):
    self.generate_ninja_script(GN_ARGS_LINUX)
    self.build_target()

    if self.target == STELLITE_QUIC_SERVER:
      return [os.path.join(self.build_output_path, self.target)]

    output_list = []
    output_list.append(self.package_target())

    if self.target == STELLITE_HTTP_CLIENT:
      output_list.extend(self.stellite_http_client_header_files)

    if self.target == TRIDENT_HTTP_CLIENT:
      output_list.extend(self.trident_http_client_header_files)

    return output_list

  def package_target(self):
    if self.target_type == STATIC_LIBRARY:
      return self.link_static_library()
    if self.target_type == SHARED_LIBRARY:
      return self.link_shared_library()

    raise Exception('invalid target type error')

  def link_static_library(self):
    library_name = 'lib{}.a'.format(self.target)
    library_path = os.path.join(self.build_output_path, library_name)

    command = [ 'ar', 'rsc', library_path, ]
    for filename in self.pattern_files(self.build_output_path, '*.a',
                                       LINUX_EXCLUDE_OBJECTS):
      command.append(filename)
    self.execute(command)

    return library_path

  def link_shared_library(self):
    library_name = 'lib{}.so'.format(self.target)
    library_path = os.path.join(self.build_output_path, library_name)

    command = [
      self.clang_compiler_path,
      '-shared',
      '-Wl,--fatal-warnings',
      '-fPIC',
      '-Wl,-z,noexecstack',
      '-Wl,-z,now',
      '-Wl,-z,relro',
      '-Wl,--no-as-needed',
      '-lpthread',
      '-Wl,--as-needed',
      '-fuse-ld=gold',
      '-Wl,--icf=all',
      '-pthread',
      '-m64',
      '-Wl,--export-dynamic',
      '-o', library_path,
      '-Wl,-soname="{}"'.format(library_name),
    ]

    for filename in self.pattern_files(self.build_output_path, '*.a',
                                       LINUX_EXCLUDE_OBJECTS):
      command.append(filename)

    self.execute(command)
    return library_path

  def fetch_toolchain(self):
    return True


class WindowsBuild(BuildObject):
  """windows build"""
  def __init__(self, target, target_type, verbose=False):
    super(self.__class__, self).__init__(target, target_type, WINDOWS,
                                         verbose=verbose)
    self._vs_path = None
    self._vs_version = None
    self._sdk_path = None
    self._sdk_dir = None
    self._runtime_dirs = None

  @property
  def python_path(self):
    return os.path.join(self.depot_tools_path, 'python276_bin', 'python.exe')

  @property
  def dependency_directories(self):
    return WINDOWS_DEPENDENCY_DIRECTORIES

  @property
  def vs_path(self):
    if hasattr(self, '_vs_path'):
      return self._vs_path

    self.get_vs_toolchain()
    return self._vs_path

  @property
  def sdk_path(self):
    if hasattr(self, '_sdk_path'):
      return self._sdk_path
    self.get_vs_toolchain()
    return self._sdk_path

  @property
  def vs_version(self):
    if hasattr(self, '_vs_version'):
      return self._vs_version
    self.get_vs_toolchain()
    return self._vs_version

  @property
  def wdk_dir(self):
    if hasattr(self, '_wdk_dir'):
      return self._wdk_dir
    self.get_vs_toolchain()
    return self._wdk_version

  @property
  def runtime_dirs(self):
    if hasattr(self, '_runtime_dirs'):
      return self._runtime_dirs
    self.get_vs_toolchain()
    return self._runtime_dirs()

  @property
  def tool_wrapper_path(self):
    return os.path.join(self.chromium_src_path, 'build', 'toolchain', 'win',
                        'tool_wrapper.py')

  @property
  def build_response_file_path(self):
    return os.path.join(self.build_output_path, 'stellite_build.rsp')

  def get_vs_toolchain(self):
    command = [
      self.python_path,
      os.path.join(self.chromium_src_path, 'build', 'vs_toolchain.py'),
      'get_toolchain_dir'
    ]

    env = os.environ.copy()
    env['DEPOT_TOOLS_WIN_TOOLCHAIN'] = '0'
    job = subprocess.Popen(command, stdout=subprocess.PIPE, env=env)
    job.wait()
    stdout = job.stdout.read().strip()

    toolchain = dict()
    for toolchain_line in stdout.split('\n'):
      unpacked = map(lambda x : x.strip().lower(), toolchain_line.split('='))
      if len(unpacked) != 2:
        continue
      toolchain[unpacked[0]] = unpacked[1]

    self._vs_path = toolchain.get('vs_path')
    self._vs_version = toolchain.get('vs_version')
    self._sdk_path = toolchain.get('sdk_path')
    self._sdk_dir = toolchain.get('wdk_dir')
    self._runtime_dirs = toolchain.get('runtime_dirs').split(';')

  def execute_with_error(self, command, cwd=None, env=None):
    print('Running: %s' % (' '.join(pipes.quote(x) for x in command)))

    env = env or os.environ.copy()
    env['DEPOT_TOOLS_WIN_TOOLCHAIN'] = '0'
    job = subprocess.Popen(command, cwd=cwd, env=env, shell=True)
    return job.wait() == 0

  def copy_stellite_code(self):
    """copy stellite code to buildspace"""
    stellite_buildspace_path = os.path.join(self.buildspace_src_path,
                                            'stellite')
    if os.path.exists(stellite_buildspace_path):
      shutil.rmtree(stellite_buildspace_path)
    copy_tree(self.stellite_path, stellite_buildspace_path)

    trident_buildspace_path  = os.path.join(self.buildspace_src_path,
                                            'trident_stellite')
    if os.path.exists(trident_buildspace_path):
      shutil.rmtree(trident_buildspace_path)
    copy_tree(self.trident_path, trident_buildspace_path)

  def package_target(self):
    if self.target_type == STATIC_LIBRARY:
      self.copy_output_files('*.lib')
    if self.target_type == SHARED_LIBRARY:
      self.copy_output_files('*.dll')
    raise Exception('invalid target type error')

  def copy_output_files(self, pattern):
    if os.path.exists(self.output_path):
      shutil.rmtree(self.output_path)
    os.makedirs(self.output_path)

    for filename in self.pattern_files(self.build_output_path, pattern):
      shutil.copy(os.path.join(self.build_output_path, filename),
                  self.output_path)

  def build(self):
    is_component = 'false' if self.target_type == STATIC_LIBRARY else 'true'
    gn_args = GN_ARGS_WINDOWS.format(is_component)
    self.generate_ninja_script(gn_args=gn_args)
    self.build_target()
    return [self.package_target()]


def main(args):
  """main entry point"""
  options = option_parser(args)

  build = build_object(options)
  if not build.check_target_platform():
    print('invalid platform error: {}'.format(options.target_platform))
    sys.exit(1)

  if options.action == CLEAN:
    build.clean()
    return 0

  build.fetch_chromium()
  build.synchronize_chromium_tag()
  build.synchronize_buildspace()

  if options.action == BUILD:
    if os.path.exists(build.output_path):
      shutil.rmtree(build.output_path)
    os.makedirs(build.output_path)

    output_files = build.build()
    for output_file_path in output_files:
      shutil.copy(output_file_path, build.output_path)
    return 0
  return 1


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
