project(
  'nv',
  'c',
  meson_version : '>= 1.3.0',
  version : files('.version'),
  default_options : [
    'c_std=c23,c2y,c2x,gnu2x,gnu23,gnu2y',
    'werror=true',
    'b_ndebug=if-release',
    # 'default_library=static',
    'warning_level=3'
  ],
)

add_global_arguments('-Wno-static-in-inline', language: 'c')
add_global_arguments('-Wno-gnu-statement-expression', language: 'c')






hedley_dep = dependency('hedley')



dependencies = [
  hedley_dep,
]

inc = include_directories(
  'include',
  'include/core',
)

core_lib = static_library(
  'nv_core',
  files(
    'src/core/log.c'
  ),
  include_directories: inc,
  dependencies: dependencies,
  install: true,
  pic: true,
  c_pch: 'pch/nv_pch.h',
)

core_dep = declare_dependency(
  include_directories: inc,
  link_with: core_lib,
)

dependencies += core_dep

mimalloc_proj = subproject('mimalloc')
mimalloc_dep = mimalloc_proj.get_variable('mi_dep')

dependencies += mimalloc_dep

memory_lib = static_library(
  'nv_memory',
  files(
    'src/memory/cstr.c',
    'src/memory/alloc.c',
    'src/memory/arena.c',
    'src/memory/gpa.c',
  ),
  include_directories: inc,
  dependencies: dependencies,
  install: true,
  pic: true,
  c_pch: 'pch/nv_pch.h',
)

memory_dep = declare_dependency(
  include_directories: inc,
  link_with: memory_lib
)


dependencies += memory_dep


lib = library(
  'nv',
  # 'src/main.c',
  include_directories: inc,
  c_pch: 'pch/nv_pch.h',
  dependencies : dependencies,
  install : true,
)


subdir('tests')

# test('basic', exe)
