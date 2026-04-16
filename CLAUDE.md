# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Commands

```sh
make          # debug build (default: -ggdb -O0 -rdynamic)
make release  # optimized build (-O3 -march=native -DNDEBUG)
make test     # build and run the test suite
make clean    # remove obj/, envwalk binary, and test_runner
```

The compiler is `gcc` with `-std=c23 -Wall -Wextra -Wpedantic -Werror`. All warnings are treated as errors.

## Architecture

`envwalk` is a single C binary that manages per-directory `.env` file loading in shells. The shell hooks (`hook.zsh`, `hook.bash`) are embedded into the binary at link time via `objcopy` and printed by `envwalk hook <shell>`.

**Core flow:**
- `envwalk` (no args) → `run()`: walks from CWD to root, finds allowed directories, parses their `.env` files, and prints `export KEY="value"` lines to stdout. The shell hook `eval`s this output before each command.
- `envwalk cd <old_path>` → `chpwd()`: diffs old vs new path to determine which variables to unset, then calls `run()`. The shell hook `eval`s this on directory change.

**Module breakdown:**
- `envwalk.c` — entry point, `main()`, `run()`, `chpwd()`, hook embedding/printing
- `config.c/h` — reads/writes `~/.config/envwalk`; manages the allowlist of directories
- `dotenv.c/h` — parses `.env` files into `Variables` (a dynamic array of `KeyValuePair`)
- `cli.c/h` — parses `argv` into `Params` (`Action` enum + optional `text`)
- `path.c/h` — path utilities: `expand_path()` (dir, trailing slash), `expand_path_file()` (file, no trailing slash), `get_path_parts()`, `is_directory()`
- `types.c/h` — `sb_from_string_list()`: builds a `/`-joined path string from a `StringList`
- `stack_trace.c/h` — signal handler that prints a stack trace on crash
- `nob.h` — vendored single-header utility library (tsoding/nob.h v3.8.0) providing `String_View`, `String_Builder`, dynamic arrays (`da_append`), file I/O, and logging

**Key data types (from nob.h):**
- `String_View` — non-owning `{data, count}` slice
- `String_Builder` — growable `char` buffer (dynamic array)
- `StringList` — dynamic array of `char *`
- `Variables` / `KeyValuePair` — `{key: String_View, value: String_View, path: String_View}`

**Config file** is `~/.config/envwalk`, one `allowed=<path>` line per directory. Paths are stored in expanded form with a trailing slash.

## Testing

Tests live in `test.c`. The test binary is built with `-DTESTING`, which enables `config_reset_for_testing()` in `config.c`. There is no way to run a single test — all tests run together via `./test_runner`.

The hook scripts contain `%s` format specifiers; they are `printf`-formatted with the envwalk binary path at print time (see `hook_zsh()`/`hook_bash()` in `envwalk.c`).
