# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased

### Added

- Rendering underline, undercurl, strikethrough
- Simplified and improved rendering improving the responsiveness (mind :digraphs)
    - Ligatures now stay intact when crossing different highlightings
- Allow launching Gtk inspector from the application menu
- Show pango markup of the grid
- Help->About dialog
- Persistent configuration of guifont via GSettings
- GUI dialog to configure settings
- Configurable smooth scrolling
- Disconnect from a TCP session [#48](https://github.com/sakhnik/nvim-ui/issues/48)
- Install CHANGELOG.html with the application

### Fixed

- Crash when failing to start a TCP session
- List prerequisites in README.md
- List only font families when selecting the font on `set guifont=*`
- Infinite loop when another nvim-ui instance is launched
- Pass focus to the grid when a new session is launched or connected
- Occasional crash when closing a session [#46](https://github.com/sakhnik/nvim-ui/issues/46)
- Data races around global session management
- Hide inactive menu items completely

## [0.0.3] - 2022-05-08

### Added

- Switched to gtk4 to get rid of texture noise. Chunks of texts are rendered as gtk labels.
- Capture and display service output like --help or --version in the GUI window
- TCP session from the application menu
- Font zoom using keyboard shortcuts `^+`, `^-`, `^0`
- Choosing font with `set guifont=*`
- Started internationalization, added Ukrainian translation
- Hint in the title on how to reach the application menu

### Fixed

- Incorrect texture preparation when there are spaces at the end
- Improved text rendering for fancy fonts like Fira Code where glyphs may be wider than one cell
- Icon URL in chocolatey nuspec should point to a CDN
- Don't allow closing the window if neovim session is still active, resurrect the window if necessary
- Handle process crash or disconnection
- Gtk4 C++ wrappers generation from GObject introspection repository using [gir2cpp](https://github.com/sakhnik/gir2cpp)

## [0.0.2] - 2021-04-27

### Added

- Passing command line options to neovim
- Structured logging to stderr in POSIX and debug output in Windows
- Automatic generation of appimage for Linux

### Fixed

- Hide cursor when neovim is busy (fixes duplicate cursor in terminal)
- External command output isn't displayed if starts with two spaces (boost::ut test failure, for instance)

## [0.0.1] - 2021-04-17

### Added

- Cursor shapes
- Use pango and cairo for text rendering and drawing => digraphs, ligatures whatnot
- Underline, undercurl
- Change mouse pointer on busy_start/busy_stop notifications
- Publish to chocolatey.org

[unreleased]: https://github.com/sakhnik/nvim-ui/compare/v0.0.3...HEAD
[0.0.3]: https://github.com/sakhnik/nvim-ui/compare/v0.0.2...v0.0.3
[0.0.2]: https://github.com/sakhnik/nvim-ui/compare/v0.0.1...v0.0.2
[0.0.1]: https://github.com/sakhnik/nvim-ui/releases/tag/v0.0.1
