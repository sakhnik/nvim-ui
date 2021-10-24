
nvim_ui_lib_sources = [
  'AsyncExec.cpp',
  'AsyncExec.hpp',
  'BaseTexture.cpp',
  'BaseTexture.hpp',
  'Input.cpp',
  'Input.hpp',
  'IWindow.hpp',
  'Logger.cpp',
  'Logger.hpp',
  'MsgPackRpc.cpp',
  'MsgPackRpc.hpp',
  'RedrawHandler.cpp',
  'RedrawHandler.hpp',
  'Renderer.cpp',
  'Renderer.hpp',
  'Session.cpp',
  'Session.hpp',
  'SessionSpawn.cpp',
  'SessionSpawn.hpp',
  'SessionTcp.cpp',
  'SessionTcp.hpp',
  'TextureCache.cpp',
  'TextureCache.hpp',
  'Timer.cpp',
  'Timer.hpp',
  'UvLoop.cpp',
  'UvLoop.hpp',
]

nvim_ui_lib = static_library('nvim-ui-lib',
  nvim_ui_lib_sources,
  dependencies: [spdlog_dep, msgpack_dep, libuv_dep],
  cpp_pch: 'pch/all.hpp',
)

nvim_ui_lib_dep = declare_dependency(
  include_directories: '.',
)

nvim_ui_sources = [
  'main.cpp',
  'GCursor.cpp',
  'GCursor.hpp',
  'GGrid.cpp',
  'GGrid.hpp',
  'GPointer.cpp',
  'GPointer.hpp',
  'GWindow.cpp',
  'GWindow.hpp',
  'IWindowHandler.hpp',
  gtk_res,
  win_res,
]

executable('nvim-ui',
  nvim_ui_sources,
  dependencies: [spdlog_dep, gtk4_dep],
  link_with: [nvim_ui_lib],
  cpp_pch: 'pch/all-gtk.hpp',
  win_subsystem: 'windows',
  install: true)
