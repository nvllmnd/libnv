project(
  'nv',
  'c',
  meson_version : '>= 1.3.0',
  version : files('.version'),
  default_options : [
    'c_std=c23,c2y,c2x,gnu2x,gnu23,gnu2y',
    'werror=true',
    'b_ndebug=if-release',
    'b_staticpic=false',
    'default_library=static',
    'warning_level=3'
  ],
)

if not meson.is_subproject()
  add_global_arguments('-Wno-static-in-inline', language: 'c')
  add_global_arguments('-Wno-gnu-statement-expression', language: 'c')
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
  cargs += '-DNV_ALIAS_VIRTMEM_AS_ARENA=1'
endif


dependencies = [
  hedley_dep,
]

inc = include_directories(
  'include',
)

core_lib = static_library(
  'nv_core',
  files(
    'src/core/log.c',
    'src/core/buffer.c',
    'src/core/vec.c',
    'src/core/algo.c',
    'src/core/spad.c',
    'src/core/fmap.c',
    'src/core/stb_sprintf.c',
  ),
  include_directories: inc,
  dependencies: dependencies,
  install: true,
  c_pch: 'pch/nv_pch.h',
  c_args: cargs,
)

core_dep = declare_dependency(
  include_directories: inc,
  link_with: core_lib,
)

dependencies += core_dep


memory_lib = static_library(
  'nv_memory',
  files(
    'src/memory/alloc.c',
    'src/memory/virt.c',
    'src/memory/block_alloc.c',
    'src/memory/runeset.c',
    'src/memory/static_alloc.c',
  ),
  include_directories: inc,
  dependencies: dependencies,
  install: true,

  c_pch: 'pch/nv_pch.h',
  c_args: cargs,
)

memory_dep = declare_dependency(
  include_directories: inc,
  link_with: memory_lib,
)


dependencies += memory_dep


nv_lib = library(
  'nv',
  # 'src/main.c',
  include_directories: inc,
  c_pch: 'pch/nv_pch.h',
  dependencies : dependencies,
  install : true,
  c_args: cargs,
)

nv_dep = declare_dependency(
  include_directories: inc,
  link_with: nv_lib
)

meson.override_dependency('libnv', nv_dep)

dependencies += nv_dep

subdir('tests')


