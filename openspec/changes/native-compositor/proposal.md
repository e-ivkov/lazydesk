# Proposal

## Why

Two findings ended the borrowed-backend era: (1) Void's compositor packaging is unreliable for us (no Hyprland, quickshell skew-broken) so "boring binary backend" isn't on offer; (2) our window model — monocle (maximized stack), floating dialogs, manual snap, named workspaces — maps poorly onto niri's scrollable tiling: every snap is emulated over columns, and bar/palette state crosses two IPC hops. A small native compositor makes the DE one program family: the compositor hosts the facade IPC in-process, reads `lazydesk.toml` directly, and dispatches the action registry natively. Tight integration is the product; the simple model makes it cheap.

## What Changes

- New C23 compositor on wlroots 0.20 (`lazydesk-wm`): outputs, seats/input via libseat, xdg-shell + layer-shell + optional XWayland.
- Window engine exactly as specced since round one: maximize-on-map (usable area minus layer-shell reserves), dialog/transient float, manual half/quarter snap, no layout tree; stacked windows per workspace with focus cycling.
- Named archetype + ad-hoc workspaces per output (the existing workspaces spec, now native).
- **In-process IPC**: the compositor hosts the NDJSON socket itself; `lazydeskctl`, palette, bar, and agents speak to the compositor directly. `lazydeskd` (the bridge daemon) retires; its ops move in-process.
- Config: `lazydesk.toml` parsed by the existing lib (no emitter round-trip); the binding registry dispatches natively.
- Session: `lazydesk-session` execs `lazydesk-wm`; **niri stays installed as the fallback session entry** (rollback = login-menu choice).
- Verification: the nested-harness pattern (run as a Wayland client under any session) plus headless backend for CI.

Out of scope / staged: IME text-input, screencast portals (xdg-desktop-portal-wlr later), touch/tablets, multi-GPU.

## Capabilities

### New Capabilities

- `compositor` — the native window engine: mapping/placement, workspaces engine, seat/input and keymap dispatch, outputs, in-process IPC hosting, crash/respawn contract, backend feature gates (layer-shell, XWayland).

### Modified Capabilities

(none yet — `lazydesk-core` establishes the shared capabilities first; after both archive, follow-ups retire the niri emitter/bridge artifacts.)

## Impact

- New `wm/` component in the monorepo; everything else survives verbatim: config lib, binding registry, `lazydeskctl`, `lazydesk` CLI, themes, tests, session entry (backend swap), quickshell shell (runs on our layer-shell).
- Build deps: wlroots 0.20 (packaged on Void), wayland, xkbcommon, libinput (all binary packages).
- The `niri.kdl` emitter and `lazydeskd` become the fallback adapter path.
