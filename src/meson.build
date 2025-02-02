sources = [
  'stack.cpp',
]

deps = [hwloc_dep, thread_dep]

libfork_lib = library(
  'libfork',
  sources,
  include_directories: inc,
  dependencies: deps,
  install: true,
)
