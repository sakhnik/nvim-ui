# neovim-sdl2

NeoVim UI implemented in SDL2

I need an easy deployable low latency UI for NeoVim in Windows. It should be both simple and flexible. Turns out, it's quite easy to code after experiments with SpyUI.

The required font DejaVuSansMono.ttf is ubiquitously available in Linux. In Windows, they it could be installed with [chocolatey](https://community.chocolatey.org/packages/dejavufonts): `choco install dejavufonts`.

# TODO

 - [x] Cursor shapes
 - [x] Use pango and cairo for text rendering and drawing => digraphs, ligatures whatnot
 - [x] Underline, undercurl
 - [ ] Pass command line arguments to neovim
 - [ ] Add configuration file (property tree)
 - [ ] Log to a file instead of stdout
 - [ ] Properly close the editor when the window is about to be closed
 - [ ] Allow selecting GUI font
 - [ ] Mouse support
 - [ ] Fast zoom with mouse wheel
 - [ ] Automatic testing
 - [ ] Handle special keys like arrows
 - [ ] Handle the rest of highlight attributes
 - [ ] Handle errors and failures, suggest restarting nvim on crash
 - [ ] Consider externalizing popup menus to add scrollbars
 - [ ] Configuration of fonts and behaviour tweaking (consider using lua)
 - [ ] Change mouse pointer on busy_start/busy_stop notifications
 - [ ] Setup automatic releases (win32, appimage)
 - [ ] Track mode_info
