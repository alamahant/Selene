# Changelog

All notable changes to Selene will be documented in this file.

## [1.0.2] - 2025-08-15
### Changed
- Reverted back to using `~/Documents/Selene` as Selene's data location after Flathub graciously granted the `--filesystem=xdg-documents` permission
- Improved mute/unmute functionality to include the startup welcome tone

## [1.0.1] - 2025-08-13
### Fixed
- DocumentRoot directory now points to correct Flatpak sandboxed location
- Contacts directory path corrected for sandboxed environment

## [1.0.0] - 2025-08-13
### Added
- Initial release of Selene on Flathub
- Tor-based peer-to-peer messaging and file sharing
- End-to-end RSA encryption with multiple key lengths (2048, 4096, 8192 bits)
- Ephemeral Tor hidden service HTTP file sharing
- Configurable Tor circuits and ports
- Contact management with import/export functionality
- Rich messaging features including emojis and file transfers
- Advanced logging system and chat history management
- Password protection at startup
- Cross-platform Qt interface
