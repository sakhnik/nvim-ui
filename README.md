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
