# nvim-ui

NeoVim UI implemented in C++ using gtk4.
Gtk C++ wrappers are generated from [GObject introspection](https://gi.readthedocs.io/en/latest/).
See the related project [sakhnik/gir2cpp](https://github.com/sakhnik/gir2cpp).

## Why yet another NeoVim GUI?

I need an easy deployable low latency UI for NeoVim in Windows. It should be both simple and flexible.
After evaluation of the available ones from the [related projects](https://github.com/neovim/neovim/wiki/Related-projects#gui),
I had to rule out almost every single one. The criteria were:

- No Electron (show stopper)
- Ability to run in [KVM](https://www.linux-kvm.org/page/Main_Page)
- Proper text rendering, for instance, with combining characters: на́голос

Turns out, it's quite easy to code after experiments with [SpyUI](https://github.com/sakhnik/nvim-gdb/blob/master/test/spy_ui.py).

## Installation

### Linux

The required font DejaVuSansMono.ttf is ubiquitously available in Linux. 

```
./scripts/build.sh
```

### Windows

In Windows, the required font could be installed with [chocolatey](https://community.chocolatey.org/packages/dejavufonts): `choco install dejavufonts`.

```
choco install neovim --pre
choco install dejavufonts
choco install nvim-ui
```
