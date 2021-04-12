# neovim-sdl2

NeoVim UI implemented in C++ using SDL2, pango/cairo.

## Why yet another NeoVim GUI?

I need an easy deployable low latency UI for NeoVim in Windows. It should be both simple and flexible.
After evaluation of the available ones from the [related projects](https://github.com/neovim/neovim/wiki/Related-projects#gui),
I had to rule out almost every single one. The criteria were:

- No Electron (show stopper)
- Responsiveness ⇒ no fancy runtimes
- Ability to run in [KVM](https://www.linux-kvm.org/page/Main_Page)
- Proper text rendering, for instance, with combining characters: на́голос

Turns out, it's quite easy to code after experiments with [SpyUI](https://github.com/sakhnik/nvim-gdb/blob/master/test/spy_ui.py).

## Installation

### Linux

The required font DejaVuSansMono.ttf is ubiquitously available in Linux. 

```
mkdir BUILD
cd BUILD
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

### Windows

In Windows, the required font could be installed with [chocolatey](https://community.chocolatey.org/packages/dejavufonts): `choco install dejavufonts`.

```
choco install neovim --pre
choco install dejavufonts
choco install .\neovim-sdl2.0.0.1.nupkg
```

## TODO

  - [x] Cursor shapes
  - [x] Use pango and cairo for text rendering and drawing => digraphs, ligatures whatnot
  - [x] Underline, undercurl
  - [x] Change mouse pointer on busy_start/busy_stop notifications
  - [ ] Pass command line arguments to neovim
  - [ ] Publish to chocolatey.org
  - [ ] Add configuration file (property tree)
  - [ ] Log to a file instead of stdout
  - [ ] Properly close the editor when the window is about to be closed
  - [ ] Allow selecting GUI font
  - [ ] Mouse support (+ mouse_on, mouse_off)
    - [ ] mouse_on, mouse_off
    - [ ] highlight URLs like in terminals
  - [ ] Fast zoom with mouse wheel
  - [ ] Automatic testing
  - [ ] Handle special keys like arrows
  - [ ] Handle the rest of highlight attributes
  - [ ] Handle errors and failures, suggest restarting nvim on crash
  - [ ] Consider externalizing popup menus to add scrollbars
  - [ ] Configuration of fonts and behaviour tweaking (consider using lua)
  - [ ] Setup automatic releases (win32, appimage)
  - [ ] Track mode_info
