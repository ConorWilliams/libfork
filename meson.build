project(
  'libfork',
  'cpp',
  version: run_command('python3', 'scripts/parse_version.py', check: true).stdout().strip(),
  license: 'Mozilla Public License Version 2.0',
  license_files: 'LICENSE.md',
  default_options: [
    'warning_level=3',
    'cpp_std=c++23',
  ],
)

# === Dependencies ===

hwloc_dep = dependency('hwloc')
thread_dep = dependency('threads')

assert_dep = dependency('libassert', modules: ['libassert::assert'], required: false)
dwarf_dep = dependency('libdwarf', required: false)

# === Libfork ===

inc = include_directories('include')

install_subdir('include/libfork', install_dir: 'include')

libfork_sources = [
  'src/stack.cpp',
]

lib_args = []

if assert_dep.found()
  lib_args += ['-DLF_USE_LIBASSERT']
endif

libfork_lib = library(
  meson.project_name(),
  libfork_sources,
  include_directories: inc,
  dependencies: [hwloc_dep, thread_dep, assert_dep, dwarf_dep],
  install: true,
  version: meson.project_version(),
  soversion: meson.project_version().split('.')[0],
  cpp_args: lib_args,
)

libfork_dep = declare_dependency(
  include_directories: inc,
  link_with: libfork_lib,
  dependencies: [hwloc_dep, thread_dep, assert_dep, dwarf_dep],
  compile_args: lib_args,
)

# === Optional ===

if get_option('lf_test')
  subdir('test')
endif

# === Packaging ===

# TODO: Ensure that consumers of libfork have assert_dep

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  libfork_lib,
  extra_cflags: lib_args,
  # Public deps
  libraries: [thread_dep, assert_dep],
)
