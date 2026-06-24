# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Commands

```sh
make          # debug build (default: -ggdb -O0 -rdynamic)
make release  # optimized build (-O3 -march=native -DNDEBUG)
make test     # run the test suite under AddressSanitizer + LeakSanitizer + UBSan
make clean    # remove obj/, envwalk binary, and test_runner
make arch     # build Arch Linux package (requires makepkg)
make ubuntu   # build Ubuntu/Debian package (requires debhelper)
make fedora   # build Fedora RPM package (requires rpmbuild)
make package  # build all three packages
```

The compiler is `gcc` with `-std=c23 -Wall -Wextra -Wpedantic -Werror`. All warnings are treated as errors.

## Architecture

`envwalk` is a single C binary that manages per-directory `.env` file loading in shells. The shell hooks (`hook.zsh`, `hook.bash`) are embedded into the binary at link time via `objcopy` and printed by `envwalk hook <shell>`.

**Core flow:**
- `envwalk` (no args) → `run()`: walks from CWD to root, finds allowed directories, parses their `.env` files, and prints `export KEY="value"` lines to stdout. The shell hook `eval`s this output before each command.
- `envwalk cd <old_path>` → `chpwd()`: diffs old vs new path to determine which variables to unset, then calls `run()`. The shell hook `eval`s this on directory change.

**Module breakdown:**
- `src/envwalk.c` — entry point, `main()`, `run()`, `chpwd()`, hook embedding/printing
- `src/config.c/h` — reads/writes `~/.config/envwalk`; manages the allowlist of directories
- `src/dotenv.c/h` — parses `.env` files into `Variables` (a dynamic array of `KeyValuePair`)
- `src/cli.c/h` — parses `argv` into `Params` (`Action` enum + optional `text`)
- `src/path.c/h` — path utilities: `expand_path()` (dir, trailing slash), `expand_path_file()` (file, no trailing slash), `get_path_parts()`, `is_directory()`
- `src/types.c/h` — `sb_from_string_list()`: builds a `/`-joined path string from a `StringList`
- `src/stack_trace.c/h` — signal handler that prints a stack trace on crash
- `src/third-party/nob.h` — vendored single-header utility library (tsoding/nob.h v3.8.0) providing `String_View`, `String_Builder`, dynamic arrays (`da_append`), file I/O, and logging
- `src/hooks/hook.zsh`, `src/hooks/hook.bash` — shell hook scripts embedded into the binary via `objcopy`

**Packaging:**
- `packaging/arch/PKGBUILD` — Arch Linux package
- `packaging/ubuntu/` — Ubuntu/Debian package control files (`control`, `changelog`, `rules`, `compat`, `copyright`, `install`)
- `packaging/fedora/envwalk.spec` — Fedora RPM spec (built via `make fedora`; output under `packaging/fedora/build/`)

**Key data types (from nob.h):**
- `String_View` — non-owning `{data, count}` slice
- `String_Builder` — growable `char` buffer (dynamic array)
- `StringList` — dynamic array of `char *`
- `Variables` / `KeyValuePair` — `{key: String_View, value: String_View, path: String_View}`

**Config file** is `~/.config/envwalk`, one `allowed=<path>` line per directory. Paths are stored in expanded form with a trailing slash.

## Testing

Tests live in `src/test.c`. The test binary is built with `-DTESTING`, which enables `config_reset_for_testing()` in `src/config.c`. There is no way to run a single test — all tests run together via `./test_runner`.

The hook scripts contain `%s` format specifiers; they are `printf`-formatted with the envwalk binary path at print time (see `hook_zsh()`/`hook_bash()` in `src/envwalk.c`).
