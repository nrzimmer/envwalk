pkgname=envwalk
pkgver=0.3
pkgrel=2
pkgdesc="envwalk is a simple environment manager for your shell"
arch=('x86_64')
license=('MIT')

source=(
  "cli.c" "cli.h"
  "config.c" "config.h"
  "dotenv.c" "dotenv.h"
  "path.c" "path.h"
  "types.c" "types.h"
  "envwalk.c"
  "nob.h"
  "Makefile"
  "hook.zsh" "hook.bash"
  "stack_trace.c" "stack_trace.h"
  "test.c"
)

sha256sums=(
  'SKIP' 'SKIP'
  'SKIP' 'SKIP'
  'SKIP' 'SKIP'
  'SKIP' 'SKIP'
  'SKIP' 'SKIP'
  'SKIP' 'SKIP' 'SKIP' 'SKIP' 'SKIP'
  'SKIP' 'SKIP'
  'SKIP'
)

build() {
  cd "$srcdir"
  make -B
}

check() {
  cd "$srcdir"
  make test
}

package() {
  cd "$srcdir"

  # install binary (adjust name if different)
  install -Dm755 envwalk "$pkgdir/usr/bin/envwalk"
}
