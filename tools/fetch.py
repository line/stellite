#!/usr/bin/env python
#
# write by @snibug

import argparse
import distutils.spawn
import fnmatch
import multiprocessing
import os
import platform
import shutil
import subprocess
import sys

ANDROID = 'android'
BUILD = 'build'
CHROMIUM = 'chromium'
CLEAN = 'clean'
CONFIGURE = 'configure'
DARWIN = 'darwin'
IOS = 'ios'
LINUX = 'linux'
MAC = 'mac'
SHARED_LIBRARY = 'shared_library'
STATIC_LIBRARY = 'static_library'
STELLITE_HTTP_CLIENT = 'stellite_http_client'
STELLITE_QUIC_SERVER = 'stellite_quic_server'
TARGET = 'target'
UBUNTU = 'ubuntu'
ALL = 'all'

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

GN_ARGS = """
is_component_build = false
"""

GN_ARGS_IOS = """
is_component_build = false
is_debug = false
enable_dsyms = false
enable_stripping = enable_dsyms
is_official_build = false
is_chrome_branded = is_official_build
use_xcode_clang = is_official_build
target_cpu = "{}"
target_os = "ios"
ios_enable_code_signing = false
symbol_level = 1
"""

GN_ARGS_IOS_SIMULATOR = """
is_component_build = false
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
  'third_party/tcmalloc',
  'third_party/tlslite',
  'third_party/yasm',
  'third_party/zlib',
  'third_party/re2',
  'tools',
  'url',
]

MAC_EXCLUDE_OBJECTS = [
  'libprotobuf_full_do_not_use.a',
  'libprotoc_lib.a',
  'libprotobuf_full.a',
]

IOS_EXCLUDE_OBJECTS = [
  'libprotobuf_full_do_not_use.a',
  'libprotoc_lib.a',
  'libprotobuf_full.a',
]

def detect_host_platform():
  """detect host architecture"""
  host_platform_name = platform.system().lower()
  if host_platform_name == DARWIN:
    return MAC
  return host_platform_name


def option_parser(args):
  """fetching tools arguments parser"""
  parser = argparse.ArgumentParser()
  parser.add_argument('--target-platform', choices=[LINUX, ANDROID, IOS, MAC],
                      help='default platform {}'.format(detect_host_platform()),
                      default=detect_host_platform())

  parser.add_argument('--target',
                      choices=[STELLITE_QUIC_SERVER, STELLITE_HTTP_CLIENT],
                      default=STELLITE_HTTP_CLIENT)

  parser.add_argument('--target-type',
                      choices=[STATIC_LIBRARY, SHARED_LIBRARY],
                      default=STATIC_LIBRARY)

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

  raise Exception('unsupported target_platform: {}'.format(options.target_type))


def which_application(name):
  return distutils.spawn.find_executable(name)


class BuildObject(object):
  """build stellite client and server"""

  def __init__(self, target=None, target_type=None, target_platform=None):
    self._target = target
    self._target_type = target_type
    self._target_platform = target_platform

    self.fetch_depot_tools()

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
    """return root absolute path"""
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

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
    return os.path.join(self.root_path, 'build')

  @property
  def gn_path(self):
    """return gn path"""
    return os.path.join(self.depot_tools_path, 'gn')

  @property
  def build_output_path(self):
    """return object file path that store a compiled files"""
    out_dir = 'out_{}'.format(self.target_platform)
    return os.path.join(self.buildspace_path, 'src', out_dir)

  @property
  def stellite_path(self):
    """return stellite code path"""
    return os.path.join(self.root_path, 'stellite')

  @property
  def modified_files_path(self):
    """return modified_files path"""
    return os.path.join(self.root_path, 'modified_files')

  @property
  def chromium_path(self):
    chromium_dir = 'chromium_{}'.format(self.target_platform)
    return os.path.join(self.third_party_path, chromium_dir)

  @property
  def chromium_branch(self):
    return self.read_chromium_branch()

  @property
  def chromium_tag(self):
    return self.read_chromium_tag()

  @property
  def clang_compiler_path(self):
    return os.path.join(self.chromium_path, 'src', 'third_party', 'llvm-build',
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

  def xcode_sdk_path(self, osx_target):
    """return xcode sdk path"""
    command = ['xcrun', '-sdk', osx_target, '--show-sdk-path']
    job = subprocess.Popen(command, stdout=subprocess.PIPE)
    job.wait()
    return job.stdout.read().strip()

  def execute_with_error(self, command, env=None, cwd=None):
    env = env or os.environ.copy()
    print('$ {}'.format(' '.join(command)))

    job = subprocess.Popen(command, env=env, cwd=cwd)
    return job.wait() == 0


  def execute(self, command, env=None, cwd=None):
    """execute shell command"""
    res = self.execute_with_error(command, env=env, cwd=cwd)
    if bool(res) == True:
      return

    print(' '.join(command))
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

  def generate_ninja_script(self, gn_args=None, gn_options=None):
    """generate ninja build script using gn."""
    gn_args = gn_args or GN_ARGS
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

    src_path = os.path.join(self.buildspace_path, 'src')
    self.execute(command, cwd=src_path)

  def packaging_target(self):
    """packaging target"""

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
      if not UBUNTU in platform.uname[3].lower():
        return False

    return True

  def check_chromium_repository(self):
    """check either chromium are valid or not"""
    if not os.path.exists(self.chromium_path):
      return False

    if not os.path.exists(os.path.join(self.chromium_path, 'src')):
      return False

    if not os.path.isdir(os.path.join(self.chromium_path, 'src')):
      return False

    if not os.path.exists(os.path.join(self.chromium_path, '.gclient')):
      return False

    if not os.path.exists(os.path.join(self.chromium_path, 'src', 'DEPS')):
      return False

    return True

  def read_chromium_branch(self):
    """read chromium git branch"""
    command = ['git', 'rev-parse', '--abbrev-ref', 'HEAD']
    working_path = os.path.join(self.chromium_path, 'src')

    try:
      job = subprocess.Popen(command, cwd=working_path, stdout=subprocess.PIPE)
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

    cwd = os.path.join(self.chromium_path, 'src')
    print('working dir: {}'.format(cwd))

    print('fetch chromium tags ...')
    self.execute(['git', 'fetch', '--tags'], cwd=cwd)

    print('reset chromium ...')
    self.execute(['git', 'reset', '--hard'], cwd=cwd)

    branch = 'chromium_{}'.format(self.chromium_tag)
    print('checkout: {}'.format(branch))

    if not self.execute_with_error(['git', 'checkout', branch], cwd=cwd):
      make_branch_command = ['git', 'checkout', '-b', branch, self.chromium_tag]
      self.execute(make_branch_command, cwd=cwd)

    print('gclient sync ...')
    command = [self.gclient_path, 'sync', '--with_branch_heads',
               '--jobs', str(multiprocessing.cpu_count() * 4)]
    self.execute(command, cwd=cwd)

  def synchronize_buildspace(self):
    """synchronize code for building libchromium.a"""
    src_path = os.path.join(self.buildspace_path, 'src')
    if not os.path.exists(src_path):
      print('make {} ...'.format(src_path))
      os.makedirs(src_path)

    for target_dir in CHROMIUM_DEPENDENCY_DIRECTORIES:
      if os.path.exists(os.path.join(src_path, target_dir)):
        continue
      print('copy chromium {} ...'.format(target_dir))
      shutil.copytree(os.path.join(self.chromium_path, 'src', target_dir),
                      os.path.join(src_path, target_dir))

    print('copy .gclient ...')
    shutil.copy(os.path.join(self.chromium_path, '.gclient'),
                self.buildspace_path)

    print('copy modified_files ...')
    command = ['cp', '-r', self.modified_files_path + '/' , src_path]
    os.system(' '.join(command))

    print('copy chromium/src/.gn ...')
    command = ['cp', os.path.join(self.chromium_path, 'src', '.gn'), src_path]
    os.system(' '.join(command))

    print('copy stellite files...')
    command = ['ln', '-s', self.stellite_path]
    self.execute_with_error(command, cwd=src_path)

  def build_target(self):
    command = ['ninja', '-C', self.build_output_path, self.target]
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
    if os.path.exists(self.buildspace_path):
      shutil.rmtree(self.buildspace_path)

  def package_target(self):
    raise NotImplementedError()


class AndroidBuild(BuildObject):
  """android build"""
  def __init__(self, target, target_type):
    super(self.__class__, self).__init__(target, target_type, ANDROID)

  def build(self):
    pass

  def fetch_toolchain(self):
    self.write_gclient(GCLIENT_ANDROID)
    self.gclient_sync()
    return True

  def package_target(self):
    pass


class MacBuild(BuildObject):
  """mac build"""
  def __init__(self, target, target_type):
    super(self.__class__, self).__init__(target, target_type, MAC)

  def build(self):
    self.generate_ninja_script()
    self.build_target()
    self.package_target()

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
      '-arch', '-x86_64',
    ]

    for filename in self.pattern_files(self.build_output_path, '*.a',
                                       MAC_EXCLUDE_OBJECTS):
      command.append('-Wl,-force_load,{}'.format(filename))

    library_name = 'lib{}.dylib'.format(self.target)
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
    return library_path

  def link_static_library(self):
    library_name = 'lib{}.a'.format(self.target)
    return os.path.join(self.build_output_path, 'obj', 'stellite',
                        library_name)


class IOSBuild(BuildObject):
  """ios build"""
  def __init__(self, target, target_type, target_arch=None):
    self._target_arch = target_arch or ALL
    super(self.__class__, self).__init__(target, target_type, IOS)

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
      build = IOSBuild(self.target, self.target_type, arch)
      build.generate_ninja_script(gn_args=GN_ARGS_IOS.format(arch),
                                  gn_options=['--check'])
      build.build_target()
      lib_list.append(build.package_target())

    self.link_fat_library(lib_list)

  def package_target(self):
    if self.target_type == STATIC_LIBRARY:
      return self.link_static_library()
    else:
      return self.link_shared_library()

  def link_shared_library(self):
    sdk_path = self.iphone_sdk_path
    if self.target_arch in ('x86', 'x64'):
      sdk_path = self.simulator_sdk_path

    lib_filename = 'lib{}.dylib'.format(self.target)
    command = [
      self.clang_compiler_path,
      '-shared',
      '-Wl,-search_paths_first',
      '-Wl,-dead_strip',
      '-miphoneos-version-min=9.0',
      '-isysroot', sdk_path,
      '-arch', self.clang_target_arch,
      '-install_name', '@loader_path/{}'.format(lib_filename),
    ]
    for filename in self.pattern_files(self.build_output_path, '*.a',
                                       IOS_EXCLUDE_OBJECTS):
      command.append('-Wl,-force_load,{}'.format(filename))

    lib_filepath = os.path.join(self.build_output_path, lib_filename)
    command.extend([
      '-o', lib_filepath,
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
    return lib_filepath

  def link_static_library(self):
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

    lib_filename = 'lib{}.a'.format(self.target)
    lib_filepath = os.path.join(self.build_output_path, lib_filename)
    command.extend([
      '-o', lib_filepath,
    ])
    self.execute(command)
    return lib_filepath

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
  def __init__(self, target, target_type):
    super(self.__class__, self).__init__(target, target_type, LINUX)

  def build(self):
    pass

  def fetch_toolchain(self):
    return True

  def package_target(self):
    pass


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
    build.build()
    return 0

  return 1


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
