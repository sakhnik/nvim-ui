# neovim-sdl2

NeoVim UI implemented in SDL2

I need an easy deployable low latency UI for NeoVim in Windows. It should be both simple and flexible. Turns out, it's quite easy to code after experiments with SpyUI.

The required font DejaVuSansMono.ttf is ubiquitously installed in Linux. In Windows, they it could be installed with [chocolatey](https://community.chocolatey.org/packages/dejavufonts): `choco install dejavufonts`.

# TODO

 - [ ] Cursor shapes
 - [ ] Use pango for text layout
 - [ ] Log to a file instead of stdout
 - [ ] Properly close the editor when the window is about to be closed
 - [ ] Allow selecting GUI font
 - [ ] Mouse support
 - [ ] Fast zoom with mouse wheel
 - [ ] Automatic testing
 - [ ] Make redrawing atomic with flush
 - [ ] Handle special keys like arrows, functional keys
 - [ ] Handle the rest of highlight attributes
 - [ ] Handle errors and failures
 - [ ] Consider externalizing popup menus to add scrollbars
 - [ ] Configuration of fonts and behaviour tweaking (consider using lua)
 - [ ] Change mouse pointer on busy_start/busy_stop notifications
 - [ ] Create window after neovim has been launched and initialized to avoid white flash during startup
 - [ ] Setup automatic releases (win32, appimage)
 - [ ] Track mode_info
