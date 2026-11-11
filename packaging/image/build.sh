#!/usr/bin/bash
# Run in the pinned Fedora/Bazzite builder, never on the live host as root.
set -euo pipefail
packaging_root=$(cd "$(dirname "$0")/../.." && pwd)
: "${SOURCE_DIR:=$packaging_root}"
: "${PACKAGING_COMMIT:=$(git -C "$packaging_root" rev-parse HEAD)}"
TARGET_FEDORA=$(cat "$packaging_root/packaging/image/fedora-version.txt")
export SOURCE_DIR PACKAGING_COMMIT TARGET_FEDORA
cd "$SOURCE_DIR"
test "$(git rev-parse HEAD)" = "$(cat "$packaging_root/packaging/image/source-commit.txt")"
: "${BUILDER_IMAGE:?Set the immutable builder image reference}"
: "${PACKAGE_REVISION:=1}"
: "${OUTPUT_DIR:=$PWD/dist}"
[[ $BUILDER_IMAGE =~ ^ghcr.io/ublue-os/bazzite@sha256:[0-9a-f]{64}$ ]]
[[ $PACKAGE_REVISION =~ ^[1-9][0-9]*$ ]]
[[ $(uname -m) == x86_64 ]]
[[ $(. /etc/os-release; echo "$VERSION_ID") == "$TARGET_FEDORA" ]]
# Do not package an unreviewed working tree or a moved submodule.
test -z "$(git status --porcelain --untracked-files=no)"
test -z "$(git submodule status --recursive | grep -E '^[+U-]' || true)"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
python3 -m unittest discover -s tests -p 'test_*.py' -v
cmake -S . -B "$work/probe" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -D_INCLUDE_SUBMODULES=ON -D_USE_XDG_CONFIG=ON -DBAZZITE_UPDATER_UI_PROBE=ON \
  -DCMAKE_INSTALL_PREFIX="$work/probe/install-root"
cmake --build "$work/probe" --parallel 2
python3 "$packaging_root/packaging/image/run_probe.py" "$SOURCE_DIR" "$work/probe" "$OUTPUT_DIR/evidence"
# A separate build tree prevents any test-only executable from being shipped.
cmake -S . -B "$work/production" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/lib/bazzite-updater-kde \
  -DKDE_INSTALL_LIBDIR=lib -DKDE_INSTALL_QMLDIR=lib/qml \
  -D_INCLUDE_SUBMODULES=ON -D_USE_XDG_CONFIG=ON -DBAZZITE_UPDATER_UI_PROBE=OFF
cmake --build "$work/production" --parallel 2
DESTDIR="$work/stage" cmake --install "$work/production"
grep -qx 'BAZZITE_UPDATER_UI_PROBE:BOOL=OFF' "$work/production/CMakeCache.txt"
root="$work/stage/usr/lib/bazzite-updater-kde"
export LD_LIBRARY_PATH="$root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QML_IMPORT_PATH="$root/lib/qml" QML2_IMPORT_PATH="$root/lib/qml"
export XDG_DATA_DIRS="$root/share:/usr/local/share:/usr/share"
while IFS= read -r -d '' binary; do
  ldd -r "$binary" > "$work/link.log" 2>&1
  if grep -E 'not found|undefined symbol' "$work/link.log"; then exit 1; fi
done < <(find "$root" -type f \( -name 'bazzite-updater' -o -name '*.so' -o -name '*.so.*' \) -print0)
export QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software SDL_NO_SIGNAL_HANDLERS=1
export QT_QUICK_CONTROLS_STYLE=org.kde.desktop QT_STYLE_OVERRIDE=breeze
mkdir -p "$work/home"
set +e
HOME="$work/home" XDG_CONFIG_HOME="$work/home/.config" \
  timeout --kill-after=3s 8s "$root/bin/bazzite-updater" > "$work/startup.log" 2>&1
status=$?
set -e
cat "$work/startup.log"
[[ $status == 124 ]]
! grep -E 'failed to load component|is not installed|Cannot load library|undefined symbol|Type .* unavailable' "$work/startup.log"
python3 "$packaging_root/packaging/image/package.py" "$work/stage" "$OUTPUT_DIR" --builder "$BUILDER_IMAGE" --revision "$PACKAGE_REVISION"
