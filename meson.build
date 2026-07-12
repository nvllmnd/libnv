# SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
# SPDX-License-Identifier: GPL-3.0-or-later

project(
  'nv',
  ['cpp', 'c'],
  meson_version: '>= 1.3.0',
  version: files('.version'),
  default_options: [
    'cpp_std=c++20,c++23,gnu++20,gnu++23',
    'cpp_eh=none',
    'cpp_rtti=false',
    'werror=true',
    'b_ndebug=if-release',
    'b_staticpic=false',
    'default_library=static',
    'warning_level=3',
  ],
)

if not meson.is_subproject()
  add_global_arguments('-Wno-static-in-inline', language: 'c')
  # add_global_arguments('-Wno-gnu-statement-expression', language: 'cpp')
endif

hedley_dep = dependency('hedley')

cargs = []

if host_machine.system() == 'windows' or host_machine.system() == 'cygwin'
  cargs += '-DSYSTEM_WINDOWS=1'
elif host_machine.system() == 'darwin'
  cargs += '-DSYSTEM_APPLEOS=1'
elif host_machine.system() == 'emscripten'
  cargs += '-DSYSTEM_EMSCRIPTEN=1'
elif host_machine.system() == 'android'
  cargs += '-DSYSTEM_ANDROID=1'
else
  cargs += '-DSYSTEM_LINUX=1'
endif

if not meson.is_subproject()
  cargs += '-DLIBNV_INTERNAL=1'
  cargs += '-D_GNU_SOURCE=1'
  cargs += '-D_DEFAULT_SOURCE=1'
  cargs += '-D_POSIX_C_SOURCE=1'
  cargs += '-D__STDC_WANT_LIB_EXT1__=1'
  cargs += '-DLIBNV_USE_GLOBAL_INTDEFS=1'
endif

dependencies = [
  hedley_dep,
]

inc = include_directories('include', 'include/nv/deps')

core_lib = static_library(
  'nv_core',
  files(
    'src/core/algo.cpp',
    'src/core/hmap.cpp',
    'src/core/log.cpp',
    'src/core/spad.cpp',
    'src/core/stb_sprintf.cpp',
    'src/core/system.cpp',
    'src/core/vec.cpp',
  ),
  include_directories: inc,
  dependencies: dependencies,
  install: true,
  cpp_pch: 'pch/nv_pch.hpp',
  cpp_args: cargs,
)

core_dep = declare_dependency(
  include_directories: inc,
  link_with: core_lib,
)

dependencies += core_dep

memory_lib = static_library(
  'nv_memory',
  files(
    'src/memory/alloc.cpp',
    'src/memory/fmap.cpp',
    'src/memory/heap.cpp',

    'src/memory/static_alloc.cpp',
    'src/memory/vmem.cpp',
  ),
  include_directories: inc,
  dependencies: dependencies,
  install: true,

  cpp_pch: 'pch/nv_pch.hpp',
  cpp_args: cargs,
)

memory_dep = declare_dependency(
  include_directories: inc,
  link_with: memory_lib,
)

dependencies += memory_dep

nv_lib = library(
  'nv',
  # 'src/main.cpp',
  include_directories: inc,
  cpp_pch: 'pch/nv_pch.hpp',
  dependencies: dependencies,
  install: true,
  cpp_args: cargs,
)

nv_dep = declare_dependency(include_directories: inc, link_with: nv_lib)

meson.override_dependency('libnv', nv_dep)

dependencies += nv_dep

# subdir('tests')
