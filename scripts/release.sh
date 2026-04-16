#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PKGBUILD="$REPO_ROOT/packaging/arch/PKGBUILD"
ARCH_CHANGELOG="$REPO_ROOT/packaging/arch/CHANGELOG"
UBUNTU_CHANGELOG="$REPO_ROOT/packaging/ubuntu/changelog"

trap 'make clean -C "$REPO_ROOT" 2>/dev/null' EXIT

usage() {
  echo "Usage: $(basename "$0") [--dry-run|--preview] <major|minor|fix>"
  echo "  --dry-run   show diff and revert changes"
  echo "  --preview   show diff and keep changes without committing"
  exit 1
}

DRY_RUN=0
PREVIEW=0
BUMP=""

for arg in "$@"; do
  case "$arg" in
    --dry-run|-n) DRY_RUN=1 ;;
    --preview|-p) PREVIEW=1 ;;
    major|minor|fix) BUMP="$arg" ;;
    *) usage ;;
  esac
done

[ -z "$BUMP" ] && usage

# Ensure clean working tree
cd "$REPO_ROOT"
if ! git diff --quiet || ! git diff --cached --quiet; then
  echo "Error: dirty working tree — commit or stash changes first"
  exit 1
fi

# Run tests and verify release build
echo "Running tests..."
make test
echo "Building release..."
make release
echo "All checks passed."
echo ""

# Read current version
CUR_VER=$(grep '^pkgver=' "$PKGBUILD" | cut -d= -f2)
CUR_REL=$(grep '^pkgrel=' "$PKGBUILD" | cut -d= -f2)

MAJOR="${CUR_VER%%.*}"
MINOR="${CUR_VER##*.}"

# Compute new version
case "$BUMP" in
  major) NEW_VER="$((MAJOR+1)).0"; NEW_REL=1 ;;
  minor) NEW_VER="${MAJOR}.$((MINOR+1))"; NEW_REL=1 ;;
  fix)   NEW_VER="$CUR_VER"; NEW_REL=$((CUR_REL+1)) ;;
esac

echo "Bumping $CUR_VER-$CUR_REL → $NEW_VER-$NEW_REL ($BUMP)"

# --- Update PKGBUILD version lines ---
sed -i "s/^pkgver=.*/pkgver=$NEW_VER/" "$PKGBUILD"
sed -i "s/^pkgrel=.*/pkgrel=$NEW_REL/" "$PKGBUILD"

# --- Recompute sha256sums if source=() has entries ---
# Source entries are basenames; search for the actual file under known dirs.
find_source_file() {
  local name="$1"
  local search_dirs=("." "src" "src/hooks" "src/third-party")
  for dir in "${search_dirs[@]}"; do
    if [ -f "$REPO_ROOT/$dir/$name" ]; then
      echo "$REPO_ROOT/$dir/$name"
      return 0
    fi
  done
  echo "Error: cannot find source file '$name' in repo" >&2
  exit 1
}

mapfile -t SRC_NAMES < <(
  awk '/^source=\(\)/{next} /^source=\(/{p=1;next} p&&/^\)/{exit} p{print}' "$PKGBUILD" \
    | tr -s ' \t' '\n' | tr -d '"' | grep -v '^$'
)

if [ "${#SRC_NAMES[@]}" -gt 0 ]; then
  NEW_SUMS=()
  for name in "${SRC_NAMES[@]}"; do
    filepath="$(find_source_file "$name")"
    NEW_SUMS+=("$(sha256sum "$filepath" | awk '{print $1}')")
  done

  {
    printf 'sha256sums=(\n'
    for sum in "${NEW_SUMS[@]}"; do printf "  '%s'\n" "$sum"; done
    printf ')\n'
  } > /tmp/envwalk_shasums

  awk '
    /^sha256sums=\(/ { skip=1 }
    skip && /^\)/ { skip=0; system("cat /tmp/envwalk_shasums"); next }
    !skip { print }
  ' "$PKGBUILD" > /tmp/envwalk_PKGBUILD.new
  mv /tmp/envwalk_PKGBUILD.new "$PKGBUILD"
fi

# --- Build change list from git commits since last tag ---
LAST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || true)
if [ -z "$LAST_TAG" ]; then
  CHANGES="  * Initial release."
else
  CHANGES=$(git log --pretty=format:"  * %s" "${LAST_TAG}..HEAD")
  [ -z "$CHANGES" ] && CHANGES="  * No changes."
fi

# --- Prepend entry to packaging/arch/CHANGELOG ---
DATE_SHORT=$(date +%Y-%m-%d)
NEW_ARCH_ENTRY="$NEW_VER-$NEW_REL ($DATE_SHORT)
$CHANGES

"
if [ -f "$ARCH_CHANGELOG" ]; then
  printf '%s' "$NEW_ARCH_ENTRY" | cat - "$ARCH_CHANGELOG" > /tmp/envwalk_CHANGELOG.new
  mv /tmp/envwalk_CHANGELOG.new "$ARCH_CHANGELOG"
else
  printf '%s' "$NEW_ARCH_ENTRY" > "$ARCH_CHANGELOG"
fi

# --- Prepend entry to packaging/ubuntu/changelog ---
DATE_RFC=$(date -R)
NEW_UBUNTU_ENTRY="envwalk ($NEW_VER-$NEW_REL) unstable; urgency=low

$CHANGES

 -- Natanael Rodrigo Zimmer <nrzimmer@gmail.com>  $DATE_RFC

"
printf '%s' "$NEW_UBUNTU_ENTRY" | cat - "$UBUNTU_CHANGELOG" > /tmp/envwalk_ubuntu_changelog.new
mv /tmp/envwalk_ubuntu_changelog.new "$UBUNTU_CHANGELOG"

# --- Show diff ---
echo ""
GIT_PAGER=cat git diff -- packaging/arch/PKGBUILD packaging/arch/CHANGELOG packaging/ubuntu/changelog

# --- Commit, tag, push ---
if [ "$DRY_RUN" -eq 1 ]; then
  echo "Dry run — reverting changes."
  git checkout -- packaging/arch/PKGBUILD packaging/arch/CHANGELOG packaging/ubuntu/changelog
elif [ "$PREVIEW" -eq 1 ]; then
  echo "Preview — changes kept, nothing committed."
else
  read -r -p "Commit and push v$NEW_VER-$NEW_REL? [y/N] " CONFIRM
  [[ "$CONFIRM" =~ ^[Yy]$ ]] || { echo "Aborted."; git checkout -- packaging/arch/PKGBUILD packaging/arch/CHANGELOG packaging/ubuntu/changelog; exit 1; }
  git add packaging/arch/PKGBUILD packaging/arch/CHANGELOG packaging/ubuntu/changelog
  git commit -m "release version $NEW_VER-$NEW_REL"
  git tag "v$NEW_VER-$NEW_REL"
  git push
  git push origin "v$NEW_VER-$NEW_REL"
  echo ""
  echo "Released v$NEW_VER-$NEW_REL"
fi
