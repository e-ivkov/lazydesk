# Design

## Context

Decision record, round three: LazyDesk gets a native compositor. Round two chose niri for
packaged boringness; Void's packaging reality (no Hyprland, quickshell skew) plus model
mismatch (monocle vs scrollable tiling) plus the integration goal ended that. Everything
built so far survives: config lib, binding registry, `lazydeskctl`, `lazydesk` CLI, themes,
session entry, quickshell shell, tests. Niri stays installed as the fallback session.

## Goals / Non-Goals

**Goals:**

- Small C23 core implementing exactly the specced model: maximize-on-map, dialog float,
  manual snap, workspaces, keymap from the registry, in-process IPC.
- Daily-drivable on the author's machine within the staged plan; verified nested + headless.
- One program family: compositor hosts the facade socket; no bridge daemon on this path.

**Non-Goals:**

- Tiling layouts, animations framework, decorations, IME v1 (staged), portals (staged),
  touch/tablets, multi-GPU, plugins.

## Decisions

### D1: wlroots 0.20 on Void, C23, dwl-lineage
wlroots 0.20 is packaged (with 0.18/0.19 alongside) — binary deps, no source rebuilds.
Scene graph (`wlr_scene`) makes rendering near-free; our layout code only computes
boxes. References: tinywl (skeleton), dwl (seat/input/workspaces patterns, MIT).
*Alternatives: smithay (Rust — rejected per user preference), own protocol stack (no).*

### D2: The compositor links the existing lib directly
`lazydesk-wm` links `lazydesk-common`: config structs from TOML (no KDL emitter round
trip), the action registry as the keymap source, spawn-with-archetype from the same code
the bridge used. The NDJSON socket code moves from `lazydeskd` into the compositor
(wl_event_loop on the socket fd); `lazydeskd` remains only as the niri-fallback bridge.

### D3: One process, event-loop-driven IPC
Socket accepts via the compositor's event loop; op handlers run inline (state is
single-threaded — Wayland's threading rules are honored by construction); events
(window.map/focus, workspace.focus, urgency, config.reloaded) fan out to subscribers.
This is the "tight integration" made concrete: no translation, no second process.

### D4: Window model as pure geometry
Each window carries `mode: maximized | snapped(edge) | floating(box)`. Apply = compute
box from output usable area. Stack per workspace is an ordered list; focus cycle moves
the top. No trees, no solver, no persistence beyond placement.

### D5: Phased graduation, fallback forever
P1 skeleton (nested harness green): backend, outputs, xdg map/maximize/focus/close,
workspaces, keymap, IPC in-process, spawn. P2 daily-drivable: layer-shell + quickshell,
dialog float, snap, urgency, idle, multi-output, XWayland. P3: session entry flips to
`lazydesk-wm`; niri entry stays in the login menu indefinitely.

### D6: Crash story is supervisor + config immortality
Compositor state is disposable; TOML + generated artifacts are already previous-good
protected. Session supervisor (login script) respawns once, then exits to greeter.

## Risks / Trade-offs

- [wlroots API churn across minors] → pin 0.20; small adapter macros; CI compiles against
  the pinned version only.
- [Protocol long tail] → explicitly staged; nothing in P1/P2 blocks on IME/portals.
- [Single-maintainer bus factor] → the niri fallback path is kept alive in CI, so the DE
  degrades to round-two behavior rather than dying with the compositor.
- [Scope creep toward "real" compositor] → the spec's model is deliberately final; new
  window-management ideas require a change proposal, not a PR.

## Migration Plan

Session entry swaps `niri -c` for `lazydesk-wm`; `lazydeskd` still exists for the niri
fallback. Users on the fallback lose nothing; switching back is a login-menu choice.

## Open Questions

- XWayland via wlroots' built-in vs xwayland-satellite — decide at P2 when X apps are
  actually exercised (satellite matches the niri path we already ship).
- Whether `lazydesk-wm` eventually also embeds the quickshell host — deferred indefinitely;
  quickshell as layer-shell client is the contract.
