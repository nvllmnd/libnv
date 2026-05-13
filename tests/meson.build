unity_dep = dependency('unity')


dependencies += unity_dep

strings_test = executable(
  'tests_strings',
  files('../tests/strings.c'),
  include_directories: inc,
  c_pch: '../pch/nv_pch.h',
  dependencies : dependencies,
  install : true,  
  c_args: cargs,
)

test('strings', strings_test)




format_io_test = executable(
  'tests_fomat_io',
  files('../tests/format_io.c'),
  include_directories: inc,
  c_pch: '../pch/nv_pch.h',
  dependencies : dependencies,
  install : true,  
  c_args: cargs,
)

test('format_io', format_io_test)




allocators_test = executable(
  'tests_allocators',
  files('../tests/allocators.c'),
  include_directories: inc,
  c_pch: '../pch/nv_pch.h',
  dependencies : dependencies,
  install : true,  
  c_args: cargs,
)

test('allocators', allocators_test)

buff_vecs_test = executable(
  'tests_buffs_and_vecs',
  files('../tests/buff_vecs.c'),
  include_directories: inc,
  c_pch: '../pch/nv_pch.h',
  dependencies : dependencies,
  install : true,  
  c_args: cargs,
)

test('buff_vecs', buff_vecs_test)

