#!/usr/bin/env bash
#
# Builds Salty OS for every supported model, and copies the results in
# release/ at the root of the repository, with the model in the file name:
#   epsilon/output/release/n0120/userland.onboarding.allow3rdparty.B.dfu
#   -> release/userland.onboarding.allow3rdparty.B.n0120.dfu
#
# Usage:
#   tools/salty_build.sh              # builds n0120, n0115 and n0110
#   tools/salty_build.sh n0120 n0110  # builds only the given models

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EPSILON_DIR="$ROOT_DIR/epsilon"
RELEASE_DIR="$ROOT_DIR/release"

TARGET="userland.onboarding.allow3rdparty.B"
EXTENSION="dfu"
DEFAULT_PLATFORMS=(n0120 n0115 n0110)

if [ "$#" -gt 0 ]; then
  PLATFORMS=("$@")
else
  PLATFORMS=("${DEFAULT_PLATFORMS[@]}")
fi

if command -v nproc >/dev/null 2>&1; then
  JOBS="$(nproc)"
else
  JOBS="$(sysctl -n hw.ncpu)"  # macOS
fi

mkdir -p "$RELEASE_DIR"
cd "$EPSILON_DIR"

for platform in "${PLATFORMS[@]}"; do
  echo "==> Building $TARGET for $platform"
  make -j"$JOBS" PLATFORM="$platform" EMBED_EXTRA_DATA=1 "$TARGET.$EXTENSION"

  built="output/release/$platform/$TARGET.$EXTENSION"
  if [ ! -f "$built" ]; then
    echo "Error: $EPSILON_DIR/$built was not generated" >&2
    exit 1
  fi
  released="$RELEASE_DIR/$TARGET.$platform.$EXTENSION"
  cp "$built" "$released"
  echo "==> $released"
done

echo "Done: $(ls "$RELEASE_DIR" | wc -l) file(s) in $RELEASE_DIR"
