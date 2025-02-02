project(
  'libfork',
  'cpp',
  version: run_command('python3', 'scripts/parse_version.py', check: true).stdout().strip(),
  default_options: [
    'warning_level=3',
    'cpp_std=c++23',
  ],
)

# === Dependencies ===

hwloc_dep = dependency('hwloc')
thread_dep = dependency('threads')

# === Libfork ===

inc = include_directories('include')

install_subdir('include/libfork', install_dir: 'include')

libfork_sources = [
  'src/stack.cpp',
]

libfork_lib = library(
  meson.project_name(),
  libfork_sources,
  include_directories: inc,
  dependencies: [hwloc_dep, thread_dep],
  install: true,
)

# === Optional ===

if get_option('test')
  subdir('test')
endif

# === Packaging ===

pkg_mod = import('pkgconfig')

pkg_mod.generate(
  libraries: libfork_lib,
  version: meson.project_version(),
  name: meson.project_name(),
  description: 'A Library to barnicate your foos.',
)
