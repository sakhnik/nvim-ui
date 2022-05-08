# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added 

- Switched to gtk4 to get rid of texture noise. Chunks of texts are rendered as gtk labels.
- Capture and display service output like --help or --version in the GUI window
- TCP session that can be engaged by recompiling currently
- Font zoom using keyboard shortcuts `^+`, `^-`, `^0`
- Allowed choosing font with `set guifont=*`
- Started internationalization, added Ukrainian translation
- Hint in the title on how to reach the application menu

### Fixed

- Incorrect texture preparation when there are spaces at the end
- Improved text rendering for fancy fonts like Fira Code where glyphs may be wider than one cell
- Icon URL in chocolatey nuspec should point to a CDN
- Don't allow closing the window if neovim session is still active, resurrect the window if necessary
- Handle process crash or disconnection

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

[unreleased]: https://github.com/sakhnik/nvim-ui/compare/v0.0.2...HEAD
[0.0.2]: https://github.com/sakhnik/nvim-ui/compare/v0.0.1...v0.0.2
[0.0.1]: https://github.com/sakhnik/nvim-ui/releases/tag/v0.0.1
