# Changelog

All notable changes to Selene will be documented in this file.

## [1.0.4] - 2026-03-02

### Technical Updates
- **Updated KDE Runtime**:
  - Upgraded KDE Flatpak runtime to version 6.10 for improved performance and compatibility
  - Ensures better system integration and future-proof security

### Added
- **Full encryption features reintroduced**:
  - Up to RSA 8192-bit encryption for secure message exchange
  - AES-256 encryption for file protection
  - End-to-end encryption fully operational for all communications

## [1.0.3] - 2025-11-08
### Added
- Full AES file encryption alongside existing RSA message encryption is being developed.
  However until it is thorougly tested and fully stabilized, Selene's encryption fetaure is being rolled back.
  It will be made available again in a future update.
- HTTP manager dialog to facilitate downloading of shared/sent files without relying on Tor Browser
- OBFS4 and WebTunnel bridge support for Tor with manager to handle their configuration
### Changed
- Performed code polishes and improvements

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
