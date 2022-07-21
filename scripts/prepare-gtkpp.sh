#!/bin/bash -e

out_dir=cpp-generated

cd $(dirname ${BASH_SOURCE[0]})/..

# Find out the path to GObject introspection before activating
# the virtualenv
gir_dir=$(python <<END
import sys
from pathlib import Path, PurePath

root_dir = Path('/usr')
if 'mingw64' in sys.executable:
    root_dir = Path(sys.executable).parent.parent

print(str(PurePath(root_dir, 'share/gir-1.0/')))
END
)

virtualenv venv
source venv/bin/activate

if [[ ! -d src/gtkpp ]]; then
  mkdir -p src/gtkpp
fi
cd src/gtkpp

if [[ ! -d gir2cpp ]]; then
  git clone https://github.com/sakhnik/gir2cpp -b v0.0.5
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
        |
        Gio::
        |
        Gio::Application.*
        |
        Gio::Settings.*
        |
        Gdk::
        |
        Gdk::ModifierType.*
        )$""", re.VERBOSE)
config.ignore = re.compile(r"""^(
        Gtk::Print.*
        |
        GdkPixbuf::.*
        |
        Gtk::PageSetupUnixDialog
        |
        GObject::Object::compat_control
        |
        Gio::SettingsBackend.*
        )$""", re.VERBOSE)
config.out_dir = '$out_dir'
config.gir_dir = '$gir_dir'

repository = Repository(config)
repository.process('Gtk', '4.0')
repository.output()
END

cat >|meson.build <<END
gtkpp_sources = [
$(find $out_dir -name '*.[hi]pp' -printf "  '%p',\n")
]

incdir = include_directories(['gir2cpp', '$out_dir'])

gtkpp_lib = static_library('gtkpp-lib',
  gtkpp_sources,
  dependencies: [gtk4_dep],
  include_directories: incdir,
  cpp_args: '-DGIR_INLINE',
)

gtkpp_dep = declare_dependency(
  link_with: gtkpp_lib,
  include_directories: ['gir2cpp', '$out_dir'],
)
END
