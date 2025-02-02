project(
  'libfork',
  'cpp',
  version: '0.1',
  default_options: [
    'warning_level=3',
    'cpp_std=c++23',
  ],
)

# Dependencies
hwloc_dep = dependency('hwloc')
thread_dep = dependency('threads')

# Headers
inc = include_directories('include')

# Libfork
subdir('include')
subdir('src')

# Optional things

if get_option('test')
  subdir('test')
endif

# Package config
