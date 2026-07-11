# SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
#
# SPDX-License-Identifier: GPL-3.0-or-later

unity_dep = dependency('unity')

# test_files = files('data/file1.txt', 'data/file2.txt')
# install_data(test_files, install_dir: 'tests/data')

dependencies += unity_dep

strings_test = executable(
  'tests_strings',
  files('../tests/strings.cpp'),
  include_directories: inc,
  c_pch: '../pch/nv_pch.hpp',
  dependencies: dependencies,
  install: true,
  c_args: cargs,
)

test('strings', strings_test, suite: 'libnv')

core_test = executable(
  'tests_core',
  files('../tests/core.cpp'),
  include_directories: inc,
  c_pch: '../pch/nv_pch.hpp',
  dependencies: dependencies,
  install: true,
  c_args: cargs,
)

test('core-test', core_test, suite: 'libnv')

allocators_test = executable(
  'tests_allocators',
  files('../tests/allocators.cpp'),
  include_directories: inc,
  c_pch: '../pch/nv_pch.hpp',
  dependencies: dependencies,
  install: true,
  c_args: cargs,
)

test('allocators', allocators_test, suite: 'libnv')

buff_vecs_test = executable(
  'tests_buffs_and_vecs',
  files('../tests/buff_vecs.cpp'),
  include_directories: inc,
  c_pch: '../pch/nv_pch.hpp',
  dependencies: dependencies,
  install: true,
  c_args: cargs,
)

test('buff_vecs', buff_vecs_test, suite: 'libnv')
