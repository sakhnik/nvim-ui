
nvim_ui_lib_sources = [
  'MsgPackRpc.cpp',
  'MsgPackRpc.hpp',
  'AsyncExec.cpp',
  'AsyncExec.hpp',
  'RedrawHandler.cpp',
  'RedrawHandler.hpp',
  'Renderer.cpp',
  'Renderer.hpp',
  'TextureCache.cpp',
  'TextureCache.hpp',
  'Timer.cpp',
  'Timer.hpp',
  'Logger.cpp',
  'Logger.hpp',
  'Input.cpp',
  'Input.hpp',
]

nvim_ui_lib = static_library('nvim-ui-lib',
  nvim_ui_lib_sources,
  dependencies: [spdlog_dep, msgpack_dep, libuv_dep],
)

nvim_ui_lib_dep = declare_dependency(
  include_directories: '.',
)

nvim_ui_sources = [
  'main.cpp',
  'Window.cpp',
  'Window.hpp',
  gtk_res,
  win_res,
]

executable('nvim-ui',
  nvim_ui_sources,
  dependencies: [spdlog_dep, gtk4_dep],
  link_with: [nvim_ui_lib],
  win_subsystem: 'windows',
  install: true)
