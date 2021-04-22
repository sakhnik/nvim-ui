# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added 

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

[unreleased]: https://github.com/sakhnik/nvim-ui/compare/v1.1.0...HEAD
[0.0.1]: https://github.com/sakhnik/nvim-ui/releases/tag/v0.0.1
