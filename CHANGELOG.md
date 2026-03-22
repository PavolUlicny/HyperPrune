# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [v2.5.0] - 2026-03-22

### Performance

- Replaced `bitboard_has_won` mask loop with a precomputed lookup table for `MAX_MOVES` ≤ 16 (i.e. `BOARD_SIZE` ≤ 4); 512 bytes for 3×3, 64 KiB for 4×4, populated once at startup by `init_win_masks()`
- Extended lookup table to `checkWinner` for `MAX_MOVES` ≤ 16, replacing the `bitboard_did_last_move_win` mask scan with a single table load (equivalent under the precondition that the board was non-terminal before the move)

### Fixed

- Corrected `initializeBoard()` comment: `player_turn` is not used by `checkWinner()`; only `move_count` is
- Corrected `checkWinner()` comment: abbreviated return values `TIE`/`CONTINUE` changed to actual enum names `GAME_TIE`/`GAME_CONTINUE`

## [v2.4.0] - 2026-03-20

### Performance

- Added `ENABLE_MOVE_ORDERING` compile-time flag to control killer-move and history heuristics; defaults to `ON` for `BOARD_SIZE <= 4` (pruning gains outweigh O(n²) sort cost) and `OFF` for larger boards where ordering degrades performance

### CI

- Reworked test workflow: replaced matrix dimensions for `board_size` and `move_ordering` with inner shell loops, reducing job count while covering all board sizes (3–8) × both move-ordering states across every job
- Added Debug build coverage to `test` (Make), `test-cmake`, and `test-windows` jobs
- Added `--seed` / `-S` CLI smoke tests to `test`, `test-cmake`, and `test-windows` (3×3 Release runs)
- Added default `ENABLE_MOVE_ORDERING` selection validation (no explicit override) for `BOARD_SIZE=3` and `BOARD_SIZE=5` in `test`, `test-cmake`, and `test-windows`, exercising the Makefile and CMake default-derivation logic
- Expanded `build-portable` to cover all board sizes × both move-ordering states (was 3×3 only)
- Switched `test-windows` from PowerShell to Bash to fix CMake `-D` variable expansion

## [v2.3.4] - 2026-03-19

### Performance

- Portable builds (`make portable`, `make install`) now use LTO, `-fno-semantic-interposition`, matching the release build flags (minus `-march=native`)
- Default CMake Release builds now use LTO, `-funroll-loops`, and `-fno-semantic-interposition`; previously these required `-DENABLE_NATIVE_OPTIMIZATIONS=ON`

### Documentation

- Rewrote "How it works" as a step-by-step pipeline walkthrough of a single `getAiMove()` call, covering board representation, move generation, search entry point, negamax loop, transposition table, terminal conditions, move ordering, and PGO
- Trimmed Highlights section — removed redundant inline explanations already covered in "How it works"
- Added CodeQL, release version, and platforms badges to README
- Added CI/CD bullet to Highlights covering the test matrix and automated release workflow
- Added note after Requirements about 5×5+ boards being impractical for real-time play

## [v2.3.3] - 2026-03-18

### Fixed

- `make install` now runs `make clean` after installing, preventing root-owned build artifacts from blocking future `make clean` calls when installing with `sudo`
- CI: set `cancel-in-progress: false` on all workflows so new pushes queue instead of cancelling in-progress runs

### Tests

- Renamed `nx` to `x_count` in `test_restartGame_clears_board_and_resets_turn` for consistency with `o_count`

### Documentation

- Clarified that `make install` and `make uninstall` require `sudo` on systems where `/usr/local/bin` is root-owned
- Corrected PGO description in README from "compiles twice" to the accurate three-step process
- `negamax.h`: added `zobrist_init()` and `transposition_table_init()` to `getAiMove()` prerequisites

## [v2.3.2] - 2026-03-17

### Added

- `-S` short flag for `--seed` (e.g. `ttt -S 42 -s 1000`)

### Documentation

- Added "Prebuilt binaries" section to README with `chmod +x` instructions for Linux/macOS release downloads
- Added author credit to README and `--help` output

## [v2.3.1] - 2026-03-16

### Performance

- `player_to_index`: replaced conditional with `player & 1` (ASCII bit-parity)
- `getAiMove`: cache `occupied` mask to avoid recomputing `x_pieces | o_pieces` twice
- Combined throughput improvement: 3×3 release ~7.3 M games/s, PGO ~8.2 M games/s (up from ~7.1 / ~7.7)

## [v2.3.0] - 2026-03-16

### Changed

- Self-play mode now exits with code 1 and prints a diagnostic to stderr if any game is won (perfect play broken); previously continued silently
- Self-play output no longer includes the Outcomes breakdown (x/o/tie counts); all games are expected to be ties
- Self-play output condensed to a single line: `N games in X.XXX s (Y.YY M games/s)`

### Fixed

- `--help` text now documents that `--quiet` requires `--selfplay`
- Corrected inaccurate comments and documentation across the codebase

### CI

- CodeQL now runs on all branches and pull requests, not only `main`
- Simplified self-play verification steps to rely on exit code instead of parsing human-readable output
- Release workflow now runs 1M-game self-play verification before uploading binaries

## [v2.2.3] - 2026-03-15

### Fixed

- Replaced `scanf("%d")` with `fgets`+`strtol` in interactive coordinate input to eliminate undefined behaviour on out-of-range integer input

### CI

- Added CodeQL static analysis workflow (`security-extended` query suite) with weekly schedule
- Bumped `github/codeql-action` from v3 to v4 for Node.js 24 compatibility

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
