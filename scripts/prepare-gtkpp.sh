#!/bin/bash -e

virtualenv venv
source venv/bin/activate

if [[ ! -d src/gtkpp ]]; then
  mkdir -p src/gtkpp
fi
cd src/gtkpp

if [[ ! -d gir2cpp ]]; then
  git clone https://github.com/sakhnik/gir2cpp
fi

pip install -r gir2cpp/requirements.txt

gir2cpp/main.py

cat >|meson.build <<END
gtkpp_sources = [
$(find out -name '*.*' -printf "  '%p',\n")
]

gtkpp_lib_dep = declare_dependency(
  include_directories: ['gir2cpp', 'out'],
)

gtkpp_lib = static_library('gtkpp-lib',
  gtkpp_sources,
  dependencies: [gtkpp_lib_dep, gtk4_dep],
  cpp_args: '-Wno-deprecated-declarations',
)
END
