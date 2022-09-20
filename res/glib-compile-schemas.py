#!/usr/bin/env python

import os
import sys
import subprocess
from pathlib import Path

destdir = os.getenv('DESTDIR')
schemas = Path(sys.argv[1])
if destdir:
    schemas = Path(destdir).joinpath(*schemas.parts[1:])
ret = subprocess.call(f"glib-compile-schemas '{schemas}'", shell=True)
sys.exit(ret)
