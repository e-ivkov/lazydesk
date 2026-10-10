#!/bin/sh
# CLI acceptance: dry-run plans for all three distros + doctor completeness.
set -e
BIN="$1"; SRCDIR="$2"; TMP="$(mktemp -d)"
export LAZYDESK_PKGDATADIR="$SRCDIR"
trap 'rm -rf "$TMP"' EXIT

# --- dry-run plans per distro mention the right packages and install tools
"$BIN" --distro void   --dry-run install > "$TMP/void.plan"
"$BIN" --distro debian --dry-run install > "$TMP/debian.plan"
"$BIN" --distro ubuntu --dry-run install > "$TMP/ubuntu.plan"

grep -q 'xbps-install -S' "$TMP/void.plan"   || { echo "FAIL void install tool"; cat "$TMP/void.plan"; exit 1; }
grep -q 'Hyprland'        "$TMP/void.plan"   || { echo "FAIL void pkg case"; exit 1; }
grep -q 'apt-get install' "$TMP/debian.plan" || { echo "FAIL debian install tool"; exit 1; }
grep -q 'mako-notifications' "$TMP/debian.plan" || { echo "FAIL debian mako pkg name"; exit 1; }
grep -q 'apt-get install' "$TMP/ubuntu.plan" || { echo "FAIL ubuntu"; exit 1; }
grep -q 'plan OK' "$TMP/void.plan" || { echo "FAIL plan incomplete"; exit 1; }
echo "dry-run plans: OK (void/debian/ubuntu)"

# --- doctor: empty PATH -> every archetype unfilled, non-zero exit
if env PATH=/nonexistent "$BIN" doctor > "$TMP/doctor.empty" 2>&1; then
  echo "FAIL doctor should exit non-zero when nothing resolves"; exit 1
fi
for a in agent editor terminal browser messenger cockpit lazygit desk files; do
  grep -q "UNFILLED $a" "$TMP/doctor.empty" || { echo "FAIL doctor missing archetype $a"; exit 1; }
done
grep -q "fix: install package" "$TMP/doctor.empty" || { echo "FAIL doctor fix hint"; exit 1; }
echo "doctor (unfilled): OK"

# --- doctor: stub PATH with all commands -> exit 0
mkdir -p "$TMP/bin"
for c in ghostty zed chromium telegram-desktop lazydesk-cockpit lazydesk-settings thunar; do
  printf '#!/bin/sh\n' > "$TMP/bin/$c"; chmod +x "$TMP/bin/$c"
done
if ! env PATH="$TMP/bin:/usr/bin:/bin" "$BIN" doctor --archetypes > "$TMP/doctor.full" 2>&1; then
  echo "FAIL doctor should pass with all archetypes filled"; cat "$TMP/doctor.full"; exit 1
fi
grep -q "all archetypes filled" "$TMP/doctor.full" || { echo "FAIL doctor ok text"; exit 1; }
echo "doctor (filled): OK"

echo "cli acceptance: ALL OK"
