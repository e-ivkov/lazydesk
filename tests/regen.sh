#!/bin/sh
# Config model + generator acceptance: previous-good, drift, foreign takeover.
set -e
BIN="$1"; SRCDIR="$2"; TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
export LAZYDESK_PKGDATADIR="$SRCDIR"
export HOME="$TMP/home"
mkdir -p "$HOME/.config/lazydesk" "$TMP/out"
cp "$SRCDIR/config/lazydesk.toml" "$HOME/.config/lazydesk/lazydesk.toml"

# 1. valid regen -> artifacts + marker + routing + binds
"$BIN" config regen > /dev/null
test -f "$HOME/.config/lazydesk/generated/niri.kdl" || { echo "FAIL no niri.kdl"; exit 1; }
head -1 "$HOME/.config/lazydesk/generated/niri.kdl" | grep -q '^// lazydesk-managed' || { echo "FAIL marker"; exit 1; }
grep -qF 'open-on-workspace "agent"' "$HOME/.config/lazydesk/generated/niri.kdl" || { echo "FAIL archetype routing"; exit 1; }
grep -qF 'Mod+Return { spawn "lazydesk-menu"; }' "$HOME/.config/lazydesk/generated/niri.kdl" || { echo "FAIL registry bind"; exit 1; }
grep -qF 'open-maximized true' "$HOME/.config/lazydesk/generated/niri.kdl" || { echo "FAIL maximize policy"; exit 1; }
# backend validation when niri is available (it is on the dev machine / CI)
if command -v niri >/dev/null 2>&1; then
  niri validate -c "$HOME/.config/lazydesk/generated/niri.kdl" 2> "$TMP/niri.err" || { echo "FAIL niri validate:"; cat "$TMP/niri.err"; exit 1; }
  grep -q . "$TMP/niri.err" && { echo "FAIL niri validate warnings:"; cat "$TMP/niri.err"; exit 1; }
fi
grep -q 'quick-terminal-position = top' "$HOME/.config/lazydesk/generated/ghostty-lazydesk.conf" || { echo "FAIL ghostty include"; exit 1; }
grep -q 'readonly property color accent' "$HOME/.config/lazydesk/generated/shell/Theme.qml" || { echo "FAIL theme qml"; exit 1; }
echo "regen: OK"

# 2. previous-good: invalid config names the key and touches nothing
H="$HOME/.config/lazydesk/generated/niri.kdl"
BEFORE=$(md5sum "$H" | cut -d' ' -f1)
sed -i 's/^idle_minutes = .*/idle_minutes = 0/' "$HOME/.config/lazydesk/lazydesk.toml"
if "$BIN" config regen 2> "$TMP/err"; then echo "FAIL invalid config accepted"; exit 1; fi
grep -q 'idle_minutes: must be >= 1' "$TMP/err" || { echo "FAIL key not named"; cat "$TMP/err"; exit 1; }
AFTER=$(md5sum "$H" | cut -d' ' -f1)
[ "$BEFORE" = "$AFTER" ] || { echo "FAIL artifacts touched on invalid config"; exit 1; }
sed -i 's/^idle_minutes = .*/idle_minutes = 10/' "$HOME/.config/lazydesk/lazydesk.toml"
echo "previous-good: OK"

# 3. drift: hand-edit a managed file -> refuse; --force -> retake
echo "# my hand edit" >> "$H"
if "$BIN" config regen 2> "$TMP/err"; then echo "FAIL drift accepted"; exit 1; fi
grep -q 'drifted' "$TMP/err" || { echo "FAIL drift msg"; cat "$TMP/err"; exit 1; }
"$BIN" config regen --force > /dev/null
tail -1 "$H" | grep -q 'lazydesk-managed\|exec-once' || { echo "FAIL force did not restore"; exit 1; }
grep -q '# my hand edit' "$H" && { echo "FAIL hand edit survived force"; exit 1; }
echo "drift: OK"

# 4. foreign file: no marker -> refuse; --force -> take over
printf 'totally foreign\n' > "$HOME/.config/lazydesk/generated/registry.json"
if "$BIN" config regen 2> "$TMP/err"; then echo "FAIL foreign accepted"; exit 1; fi
grep -q 'foreign' "$TMP/err" || { echo "FAIL foreign msg"; exit 1; }
"$BIN" config regen --force > /dev/null || { echo "FAIL force foreign"; exit 1; }
echo "foreign: OK"

echo "regen acceptance: ALL OK"
