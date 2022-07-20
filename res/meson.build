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

schemas_dir = get_option('prefix') / get_option('datadir') / 'glib-2.0' / 'schemas'
settings_schemas = [ 'com.sakhnik.nvim-ui.gschema.xml' ]

install_data(settings_schemas, install_dir: schemas_dir)
meson.add_install_script('glib-compile-schemas', schemas_dir)

# Compile the schemas in the current directory; this is only
# useful for testing
gnome.compile_schemas(depend_files: files(settings_schemas))
