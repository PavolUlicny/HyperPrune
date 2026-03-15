# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [v2.2.2] - 2026-03-15

### CI

- Added `build-portable` job to normal CI that mirrors the exact portable build path used by the release workflow (`make portable BOARD_SIZE=3`), smoke-tests the binary, and runs 1M-game perfect-play verification on Linux and macOS
- Bumped `actions/checkout` from v4 to v6 across all workflows for Node.js 24 compatibility
- Bumped `actions/upload-artifact` and `actions/download-artifact` from v4 to v7 for Node.js 24 compatibility

### Fixed

- CLI now errors on unexpected positional arguments (e.g. `ttt foo`, `ttt -- foo`) instead of silently ignoring them and entering interactive mode
- `getAiMove` now returns (-1, -1) on invalid `aiPlayer` (not `'x'` or `'o'`) instead of producing silently wrong results

## [v2.2.1] - 2026-03-11

### Fixed

- `--selfplay` now exits with an error on invalid non-integer values instead of silently falling back to the default count
- Clarified in README that released binaries are compiled for 3×3; other board sizes require compiling from source

## [v2.2.0] - 2026-03-10

### Added

- Automated release workflow: pushing a version tag builds Linux/macOS/Windows binaries, generates `checksums.txt`, extracts release notes from CHANGELOG, and publishes the GitHub release automatically

### Changed

- `make portable` on Linux now statically links, producing a fully self-contained binary with no glibc version dependency
- Windows CMake builds now use static CRT (`/MT`), eliminating the Visual C++ Redistributable requirement
- Bumped CMake minimum required version from 3.10 to 3.15

### CI

- Test workflow restricted to branch pushes; tag pushes handled exclusively by the release workflow to avoid duplicate runs
- Release workflow now smoke-tests each binary (`--help`) before uploading, catching startup crashes or broken linker output

## [v2.1.0] - 2026-03-01

### Added

- Killer-move heuristic: try depth-local refutation moves before other moves
- History heuristic: order remaining moves by cumulative beta-cutoff count

### Changed

- Refactored bitboard and Zobrist APIs to use bit indices throughout, removing row/col from the hot path
- Expanded test suite with main-diagonal and repeated-call determinism tests

### Performance

- Significant throughput improvement from bitboard/Zobrist API refactor to bit indices (primary gain)
- Additional throughput improvement from killer-move and history heuristics

### CI

- Added `fail-fast: false` to all matrix jobs for full failure visibility

## [v2.0.0] - 2026-02-25

### Added

- Extended CI to cover all board sizes (3–8)
- Substantially expanded test suite: game logic, scenario, and edge case tests

### Changed

- Replaced Minimax with Negamax, unifying the separate high/low recursive functions into a single formulation
- Simplified transposition table API
- Renamed internal constants and game result symbols for clarity
- Simplified CLI argument parsing

### Fixed

- Transposition table correctness bug: TT entries could be reused incorrectly across `getAiMove` calls when side-to-move was not encoded in the hash

## [v1.4.0] - 2026-02-22

### Added

- Improved test suite coverage
- Expanded CI coverage

### Performance

- Performance improvements in the Minimax search

### Changed

- CLI clarity and correctness improvements
- Code and comment polishes

## [v1.3.0] - 2026-02-17

### Added

- `ENABLE_NATIVE_OPTIMIZATIONS` CMake option for `-march=native` and aggressive flags
- CMake testing on Linux and macOS in CI

### Changed

- Code and repository polishes
- Clearer CLI output and documentation
- Improved transposition table size-0 handling

## [v1.2.0] - 2026-02-16

### Added

- Cross-platform CMake build system
- MSVC and Windows support
- Windows CI testing

### Changed

- Polished transposition table probe API

## [v1.1.0] - 2026-02-15

### Added

- Perfect-play self-tests for 4×4 boards in CI

### Performance

- Throughput improvements on 3×3 and 4×4 via micro-optimizations
- Transposition table improvements

## [v1.0.0] - 2026-02-14

### Added

- Initial release
- Minimax alpha-beta search engine
- Bitboard representation for fast win detection
- Zobrist-hashed transposition table
- Interactive human vs AI game mode
- Self-play benchmarking mode (`--selfplay`)
- Makefile build system with debug, release, and PGO targets
- Unity-based test suite
