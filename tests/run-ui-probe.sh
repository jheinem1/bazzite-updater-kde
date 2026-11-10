#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
build=${1:-"$root/build-test"}
out=${2:-"$root/evidence"}
mkdir -p "$out/config/bazzite-updater" "$out/home" "$out/light" "$out/dark"
cp "$root/src/resources/KAboutData_OS.json" "$root/src/resources/rebase-targets.json" "$out/config/bazzite-updater/"
printf '[Commands]\nsystemUpdateCommand=/usr/bin/false\nallowEarlyExit=false\n[Rebase]\nenabled=true\n' > "$out/config/bazzite-updater/config.ini"
export QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software
export QT_QUICK_CONTROLS_STYLE=org.kde.desktop QT_STYLE_OVERRIDE=breeze
export XDG_CURRENT_DESKTOP=KDE KDE_FULL_SESSION=true
export XDG_CONFIG_HOME="$out/home" XDG_CONFIG_DIRS="$out/config"
export QML_IMPORT_PATH="$build/bin${QML_IMPORT_PATH:+:$QML_IMPORT_PATH}"
for theme in Light Dark; do
    name=${theme,,}
    UPDATER_UI_PROBE_DIR="$out/$name" UPDATER_UI_PROBE_SCHEME="/usr/share/color-schemes/Breeze${theme}.colors" \
        timeout 60 "$build/bin/bazzite-updater" > "$out/ui-$name.log" 2>&1
    test -s "$out/$name/ui-report.json"
    cat "$out/ui-$name.log"
done
