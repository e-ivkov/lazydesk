#!/bin/sh
# Native compositor acceptance (headless): map/maximize, workspaces engine,
# lineage routing, focus/snap/fullscreen/close ops, events, malformed input.
set -e
WM="$1"; CTL="$2"
RT="$(mktemp -d)"; export XDG_RUNTIME_DIR="$RT"
clean() { kill $WPID $SUB 2>/dev/null; pkill ghostty 2>/dev/null; rm -rf "$RT"; }
die() { echo "$@"; clean; exit 1; }
WLR_BACKENDS=headless "$WM" >"$RT/wm.log" 2>&1 & WPID=$!
sleep 1

jq_or_die() { python3 -c "import json,sys; d=json.load(sys.stdin); $1" 2>/dev/null; }

# version: native backend
"$CTL" --json version | grep -q '"backend":"lazydesk-wm"' || die "FAIL version"

# workspaces: 7 archetype workspaces from TOML
WS=$("$CTL" --json workspaces list)
echo "$WS" | jq_or_die "assert any(w['name']=='agent' and w['number']==1 for w in d)" || die "FAIL agent ws"
echo "$WS" | jq_or_die "assert any(w['name']=='lazygit' and w['number']==7 for w in d)" || die "FAIL lazygit ws"

# map: spawn a client -> window maps on active ws, focused
"$CTL" --json spawn -- ghostty >/dev/null; sleep 2
W=$("$CTL" --json windows list)
echo "$W" | jq_or_die "assert len(d)==1 and d[0]['mapped'] and d[0]['focused'] and d[0]['workspace_name']=='agent'" || die "FAIL map/maximize state: $W"

# lineage routing: spawn with archetype -> lands on that workspace (same app_id)
"$CTL" --json spawn --archetype terminal -- ghostty >/dev/null; sleep 2
W=$("$CTL" --json windows list)
echo "$W" | jq_or_die "assert any(w['workspace_name']=='terminal' for w in d)" || die "FAIL lineage routing: $W"

# events: subscriber sees workspace.focus
"$CTL" subscribe > "$RT/ev.out" 2>&1 & SUB=$!
sleep 0.5
"$CTL" workspace focus editor >/dev/null
sleep 0.5
grep -q '"event":"workspace.focus".*"name":"editor"' "$RT/ev.out" || { echo "FAIL events"; cat "$RT/ev.out"; exit 1; }

# focus round-trip keeps focused window (regression for the early-return bug)
"$CTL" workspace focus terminal >/dev/null
"$CTL" workspace focus agent >/dev/null
"$CTL" window snap left >/dev/null || die "FAIL snap left (no focused window)"
"$CTL" window snap right >/dev/null
"$CTL" window snap restore >/dev/null
"$CTL" window fullscreen >/dev/null

# close by id; compositor survives
ID=$("$CTL" --json windows list | jq_or_die "print([w['id'] for w in d if w['workspace_name']=='agent'][0])")
"$CTL" --json window close "$ID" >/dev/null; sleep 1
"$CTL" --json windows list | jq_or_die "assert not any(w['id']==$ID for w in d)" || die "FAIL close"
kill -0 $WPID || die "FAIL compositor died on close"

# malformed input survives
OUT=$(python3 - <<'PY'
import socket, os
s = socket.socket(socket.AF_UNIX)
s.connect(os.environ['XDG_RUNTIME_DIR'] + '/lazydesk/ipc.sock')
s.sendall(b'nonsense\n')
print(s.recv(4096).decode().strip())
s.close()
PY
)
echo "$OUT" | grep -q '"error":"malformed request"' || die "FAIL malformed"
kill -0 $WPID || die "FAIL compositor died on malformed"

echo "wm acceptance: ALL OK"
