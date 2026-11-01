#!/usr/bin/env bash
#
# Builds Salty OS for every supported model, and copies the results in
# release/ at the root of the repository, with the model in the file name:
#   epsilon/output/release/n0120/userland.onboarding.allow3rdparty.B.dfu
#   -> release/userland.onboarding.allow3rdparty.B.n0120.dfu
#
# Slots A and B are built for every model. In addition, n0110 gets:
#   - CUSTOM_KERNEL=1                  (slots A and B)
#       -> release/userland.onboarding.allow3rdparty.A.custom_kernel.n0110.dfu
#       -> release/userland.onboarding.allow3rdparty.B.custom_kernel.n0110.dfu
#   - CUSTOM_KERNEL=1 BIG_USERLAND=1   (slot A only)
#       -> release/userland.onboarding.allow3rdparty.A.custom_kernel.big_userland.n0110.dfu
#
# The web simulator is built too (PLATFORM=web, target epsilon.html):
#       -> release/epsilon.html and release/epsilon.js
#   (names are kept as is: epsilon.html loads epsilon.js by name)
#
# Usage:
#   tools/salty_build.sh              # builds n0120, n0115, n0110 and web
#   tools/salty_build.sh n0120 n0110  # builds only the given platforms

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EPSILON_DIR="$ROOT_DIR/epsilon"
RELEASE_DIR="$ROOT_DIR/release"

BASE_TARGET="userland.onboarding.allow3rdparty"
EXTENSION="dfu"
DEFAULT_PLATFORMS=(n0120 n0115 n0110 web)

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

echo "==> Cleaning output"
rm -rf "output"

# build_variant <platform> <tag> <slots> [MAKE_VAR=value ...]
#   platform: n0120, n0115, n0110
#   tag:      suffix put in the file name before the model (empty for none)
#   slots:    space-separated list of slots to build, e.g. "A B"
#   Remaining arguments are extra make variables (CUSTOM_KERNEL=1, ...).
#
# Slots of one variant are built in a single make call, so the flags never
# change between slots: no clean is needed there.
build_variant() {
  local platform="$1" tag="$2" slots="$3"
  shift 3
  local flags=("$@")
  local targets=() slot

  for slot in $slots; do
    targets+=("$BASE_TARGET.$slot.$EXTENSION")
  done

  # Make does not track changes of CUSTOM_KERNEL / BIG_USERLAND. Only n0110
  # is built with different flags, so only its output is wiped, each time the
  # flags change (i.e. once per variant, never between slots).
  if [ "$platform" = "n0110" ]; then
    echo "==> Cleaning output/release/$platform (flags change)"
    rm -rf "output/release/$platform"
  fi

  echo "==> Building ${targets[*]} for $platform ${flags[*]+${flags[*]}}"

  make -j"$JOBS" PLATFORM="$platform" EMBED_EXTRA_DATA=1 \
    ${flags[@]+"${flags[@]}"} "${targets[@]}"

  local built released
  for slot in $slots; do
    built="output/release/$platform/$BASE_TARGET.$slot.$EXTENSION"
    if [ ! -f "$built" ]; then
      echo "Error: $EPSILON_DIR/$built was not generated" >&2
      exit 1
    fi
    released="$RELEASE_DIR/$BASE_TARGET.$slot${tag:+.$tag}.$platform.$EXTENSION"
    cp "$built" "$released"
    echo "==> $released"
  done
}

# build_web: web simulator, generates epsilon.html and epsilon.js
build_web() {
  echo "==> Building epsilon.html for web"
  make -j"$JOBS" PLATFORM=web epsilon.html

  local file built
  for file in epsilon.html epsilon.js; do
    built="output/release/web/$file"
    if [ ! -f "$built" ]; then
      echo "Error: $EPSILON_DIR/$built was not generated" >&2
      exit 1
    fi
    cp "$built" "$RELEASE_DIR/$file"
    echo "==> $RELEASE_DIR/$file"
  done
}

for platform in "${PLATFORMS[@]}"; do
  if [ "$platform" = "web" ]; then
    build_web
    continue
  fi

  # Standard build, slots A and B
  build_variant "$platform" "" "A B"

  if [ "$platform" = "n0110" ]; then
    # Custom kernel, slots A and B
    build_variant "$platform" "custom_kernel" "A B" CUSTOM_KERNEL=1

    # Custom kernel + big userland, slot A only
    build_variant "$platform" "custom_kernel.big_userland" "A" \
      CUSTOM_KERNEL=1 BIG_USERLAND=1
  fi
done

echo "Done: $(ls "$RELEASE_DIR" | wc -l) file(s) in $RELEASE_DIR"
