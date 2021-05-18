gnome = import('gnome')

gtk_res = gnome.compile_resources(
  'res',
  'res.xml',
  source_dir: '.',
  c_name: 'res',
)

if host_machine.system() == 'windows'
  windows = import('windows')

  win_res = windows.compile_resources(
    'windows.rc',
    depend_files: 'nvim-ui.ico',
  )
else
  win_res = []
endif
