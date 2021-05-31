project('nvim-ui', ['c', 'cpp'],
  default_options: [
    'cpp_std=c++20',
    'warning_level=2',
  ],
  version: '0.0.3',
  license: 'MIT')

add_global_arguments('-DVERSION="' + meson.project_version() + '"', language: ['c', 'cpp'])

msgpack_dep = dependency('msgpack')
libuv_dep = dependency('libuv')
glib_dep = dependency('glib-2.0')
gtk4_dep = dependency('gtk4')

cmake = import('cmake')

spdlog = cmake.subproject('spdlog')
spdlog_dep = spdlog.dependency('spdlog')

subdir('res')
subdir('src')
subdir('tests')
