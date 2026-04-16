# envwalk

envwalk is a simple environment manager for your shell. It automatically loads `.env` files when you enter an allowed directory and unloads them when you leave.

## Features

- Automatically loads `.env` files from the current directory and its parents
- Unsets variables when you leave a directory
- Per-directory allowlist — only directories you explicitly allow are loaded
- `~` in values is expanded to `$HOME`
- Supports zsh and bash

## Installation

**Build from source:**
```sh
make
sudo cp envwalk /usr/local/bin/envwalk
```

**Arch Linux (PKGBUILD):**
```sh
makepkg -si
```

## Shell setup

Add to `~/.zshrc`:
```sh
eval "$(envwalk hook zsh)"
```

Add to `~/.bashrc`:
```sh
eval "$(envwalk hook bash)"
```

## Usage

| Command | Description |
|---|---|
| `envwalk allow [path]` | Allow a directory to auto-load its `.env` file |
| `envwalk deny [path]` | Remove a directory from the allowed list |
| `envwalk list` | Show all allowed directories |
| `envwalk hook <shell>` | Print the shell hook (`zsh` or `bash`) |
| `envwalk help` | Show help |

`allow` and `deny` default to the current directory when no path is given.

## .env file format

```sh
KEY=value
QUOTED="hello world"
PATH_VAR=~/projects    # ~ is expanded to $HOME
# lines starting with # or // are ignored
# duplicate keys are ignored (first wins)
```

## How it works

On every command, envwalk traverses from the current directory up to the root, loading `.env` files from all allowed directories it finds. When you change directory, variables that belonged to directories you've left are automatically unset.

## Development

```sh
make          # debug build
make release  # optimized build
make test     # run test suite
make clean    # remove build artifacts
```

Contributions are welcome!
