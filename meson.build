project('nvim-ui', ['c', 'cpp'],
  default_options: [
    'cpp_std=c++20',
    'warning_level=2',
  ],
  version: '0.1.0',
  license: 'MIT')

add_project_arguments('-DGETTEXT_PACKAGE="nvim-ui"', language:'cpp')
add_global_arguments('-DVERSION="' + meson.project_version() + '"', language: ['c', 'cpp'])

msgpack_dep = dependency('msgpack')
libuv_dep = dependency('libuv')
fmt_dep = dependency('fmt')
gtk4_dep = dependency('gtk4')

cmake = import('cmake')

spdlog = cmake.subproject('spdlog')
spdlog_dep = spdlog.dependency('spdlog')

msgpack_cxx = cmake.subproject('msgpack-c')
msgpack_cxx_dep = msgpack_cxx.dependency('msgpackc-cxx')

subdir('res')
subdir('src')
subdir('tests')
subdir('po')
