#!/bin/bash -e

out_dir=cpp-generated

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

PYTHONPATH=gir2cpp python <<END
import re
from gir2cpp.repository import Repository
from gir2cpp.config import Config

config = Config()
config.include = re.compile(r"""^(
        GObject::.*
        |
        Gtk::.*
        )""", re.VERBOSE)
config.ignore = re.compile(r"""^(
        Gtk::Print.*
        |
        GdkPixbuf::.*
        |
        Gtk::PageSetupUnixDialog
        )$""", re.VERBOSE)
config.gir_dir = '/usr/share/gir-1.0/'
config.out_dir = '$out_dir'

repository = Repository(config)
repository.process('Gtk', '4.0')
repository.output()
END

cat >|meson.build <<END
gtkpp_sources = [
$(find $out_dir -name '*.*' -printf "  '%p',\n")
]

incdir = include_directories(['gir2cpp', '$out_dir'])

gtkpp_lib = static_library('gtkpp-lib',
  gtkpp_sources,
  dependencies: [gtk4_dep],
  include_directories: incdir,
  cpp_args: '-Wno-deprecated-declarations',
)

gtkpp_dep = declare_dependency(
  link_with: gtkpp_lib,
  include_directories: ['gir2cpp', '$out_dir'],
)
END
