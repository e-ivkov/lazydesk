# Spec Delta

## Purpose

Defines the native window engine: how the compositor maps, places, and stacks windows on archetype workspaces, hosts the facade IPC in-process, handles seats/outputs, and stays recoverable — with borrowed-backend behavior (niri) as fallback only.

## ADDED Requirements

### Requirement: Maximize-on-map window engine
The compositor SHALL map every xdg-shell toplevel maximized to its output's usable area (minus layer-shell exclusive zones), focused, atop the workspace stack. It MUST apply no automatic layout. Windows requesting fullscreen SHALL be honored over the full output.

#### Scenario: Regular window opens
- **WHEN** a toplevel without dialog characteristics maps
- **THEN** it fills the usable area, takes keyboard focus, and the previous top window remains on the workspace beneath it

### Requirement: Dialog float heuristic
A toplevel whose xdg type is dialog/utility/splash or which sets a transient parent SHALL be placed floating, centered on its parent at requested size, above maximized windows; closing it returns focus to the prior window.

#### Scenario: File picker
- **WHEN** a modal file chooser maps
- **THEN** it floats centered with focus, and focus returns on dismissal

### Requirement: Manual snap without layout tree
The user SHALL be able to set the focused window to left/right/top/bottom half or any quarter, and restore maximized, via IPC and keybind. Snap state is per-window; no container or split objects exist.

#### Scenario: Two windows side by side
- **WHEN** one window is snapped left and another right on the same workspace
- **THEN** each occupies its half; restoring either returns it to maximized

### Requirement: Workspaces engine
Each output SHALL hold named archetype workspaces (from configuration) plus ad-hoc numbered ones; windows are assigned at spawn by app-id matching rules; switching changes only that output's active workspace; empty ad-hoc workspaces are cleaned up; archetype workspaces persist.

#### Scenario: Autospawned stack lands on archetypes
- **WHEN** the session autospawns the configured stack
- **THEN** each first window appears maximized on its matching archetype workspace

### Requirement: In-process IPC hosting
The compositor SHALL host the NDJSON facade socket itself (`$XDG_RUNTIME_DIR/lazydesk/ipc.sock`) exposing the same protocol as `lazydeskctl` consumes today, plus event subscription. No bridge daemon exists on this path; compositor ops execute synchronously with internal state under the compositor's own locks.

#### Scenario: lazydeskctl unchanged
- **WHEN** `lazydeskctl --json windows list` runs against the native session
- **THEN** it returns the same normalized shape as the bridge path did, without any intermediate daemon

### Requirement: Keymap dispatch from the registry
The compositor SHALL implement its keymap directly from the action registry (the same table that generates menus/cheatsheets/verbs), honoring rebinds after config reload, and passing unbound Super combinations through to focused clients that grabbed them.

#### Scenario: Rebind applies live
- **WHEN** configuration rebinds an action and reloads
- **THEN** the new combination triggers it without restart

### Requirement: Seats, outputs, and protocols
The compositor SHALL support seatd and logind seats, hotplugged outputs with per-output workspace sets, layer-shell (bar/palette/quickshell), xdg-activation (urgency), idle-notify + output power management, and optional XWayland.

#### Scenario: Quickshell bar runs natively
- **WHEN** the session starts the Quickshell shell
- **THEN** it renders on our layer-shell with exclusive zones reserved and receives workspace events

### Requirement: Crash recoverability
A compositor crash MUST disconnect clients cleanly; the session supervisor (login-level respawn) MUST restart it once automatically; on second failure the session exits to the greeter where the niri fallback entry remains selectable. State loss is bounded to window placement, never to configuration.

#### Scenario: Crash and respawn
- **WHEN** the compositor crashes once
- **THEN** it respawns via the supervisor, reloads TOML, and re-establishes the IPC socket without a re-login

### Requirement: Nested and headless verification
The compositor SHALL run nested as a Wayland client (for live verification inside any session) and headless (for CI), with the full IPC surface exercisable in both modes.

#### Scenario: CI smoke
- **WHEN** CI starts the compositor headless
- **THEN** `lazydeskctl` round-trips version, spawn, windows/workspaces queries, and focus ops without a physical seat
