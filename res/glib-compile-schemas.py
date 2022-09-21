#!/usr/bin/env python

import os
import sys
import subprocess
from pathlib import Path

destdir = os.getenv('DESTDIR')
schemas = Path(sys.argv[1])
if destdir:
    # Don't do anything temporarily in MinGW until meson is fixed
    if os.name == "nt":
        sys.exit(0)
    schemas = Path(destdir).joinpath(*schemas.parts[1:])
ret = subprocess.call(f"glib-compile-schemas '{schemas}'", shell=True)
sys.exit(ret)
