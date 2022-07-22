ut_proj = subproject('ut')
ut_dep = ut_proj.get_variable('boostut_dep')

tests_sources = [
  'Renderer.cpp',
  'test.cpp',
]

e = executable('tests',
  tests_sources,
  dependencies: [ut_dep, nvim_ui_lib_dep],
  link_with: nvim_ui_lib,
)

test('all', e)
