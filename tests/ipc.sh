#!/bin/sh
# Facade IPC acceptance: version, malformed input, unknown op, backend error, reload.
set -e
BIN_CTL="$1"; BIN_DAEMON="$2"; SRCDIR="$3"
TMP="$(mktemp -d)"; RT="$TMP/rt"; HOME_T="$TMP/home"; export TMP
trap 'kill $DPID 2>/dev/null; rm -rf "$TMP"' EXIT
mkdir -p "$RT" "$HOME_T/.config/lazydesk"
cp "$SRCDIR/config/lazydesk.toml" "$HOME_T/.config/lazydesk/lazydesk.toml"
export XDG_RUNTIME_DIR="$RT" HOME="$HOME_T" LAZYDESK_PKGDATADIR="$SRCDIR"

"$BIN_DAEMON" 2> "$TMP/daemon.log" & DPID=$!
sleep 0.4
kill -0 $DPID || { echo "FAIL daemon died"; cat "$TMP/daemon.log"; exit 1; }

# version op
"$BIN_CTL" --json version > "$TMP/v.json" && grep -q '"protocol":1' "$TMP/v.json" || { echo "FAIL version"; exit 1; }

# malformed input -> structured error, daemon survives
OUT=$(python3 - <<'PY'
import socket, os
s = socket.socket(socket.AF_UNIX)
s.connect(os.environ['XDG_RUNTIME_DIR'] + '/lazydesk/ipc.sock')
s.sendall(b'this is not json\n')
print(s.recv(8192).decode().strip())
s.close()
PY
)
echo "$OUT" | grep -q '"error":"malformed request"' || { echo "FAIL malformed: $OUT"; exit 1; }
kill -0 $DPID || { echo "FAIL daemon died on malformed input"; exit 1; }

# unknown op -> structured error
OUT=$(python3 - <<'PY'
import socket, os, json
s = socket.socket(socket.AF_UNIX)
s.connect(os.environ['XDG_RUNTIME_DIR'] + '/lazydesk/ipc.sock')
s.sendall(b'{"op":"explod"}\n')
print(s.recv(8192).decode().strip())
s.close()
PY
)
echo "$OUT" | grep -q '"error":"unknown op"' || { echo "FAIL unknown op: $OUT"; exit 1; }

# backend unavailable -> structured error + exit 1 (no niri session here)
if "$BIN_CTL" --json windows list > "$TMP/w.json" 2>/dev/null; then
  echo "FAIL backend error should exit non-zero"; exit 1
fi
grep -q '"error":"backend unavailable"' "$TMP/w.json" || { echo "FAIL backend error shape"; cat "$TMP/w.json"; exit 1; }

# config.reload works headless (regenerates artifacts)
"$BIN_CTL" config reload > /dev/null || { echo "FAIL config reload"; exit 1; }
test -f "$HOME_T/.config/lazydesk/generated/niri.kdl" || { echo "FAIL reload produced no artifacts"; exit 1; }

# spawn works (process actually starts)
"$BIN_CTL" --json spawn -- sh -c 'echo spawned > "$TMP/spawn.ok"' > /dev/null || { echo "FAIL spawn rpc"; exit 1; }
sleep 0.4
test -f "$TMP/spawn.ok" || { echo "FAIL spawn did not exec"; exit 1; }

kill $DPID 2>/dev/null
echo "ipc acceptance: ALL OK"
