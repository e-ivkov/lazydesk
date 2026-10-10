# Proposal

## Why

A solo C/Rust developer's ideal desktop — maximized-first archetype workspaces, one palette, an AI harness as first-class citizen — does not need a new compositor; it needs an opinionated, packageable DE *profile* on an established one (the Omarchy pattern, but developer-first and Pi-first instead of general-purpose). Building the policy layer as config + small components delivers the same specified behavior in days, keeps someone else's thousand contributors handling compositor hardening, and every custom component stays reusable by others.

## What Changes

This is the first working LazyDesk session, as a packageable DE on the Hyprland backend:

- **TOML as the single source of truth** (`~/.config/lazydesk/`): archetypes, bindings, window rules, spawn entries, scratchpad, harness, idle, outputs, themes, palette categories. A generator emits backend configs (hyprland.conf), the Quickshell shell config (bar, action menu, cheatsheet, notifications, OSD), the terminal include, and the keybinding registry — managed artifacts are marked and drift-checked.
- **`lazydeskd` + `lazydeskctl`: LazyDesk's own stable IPC facade** (NDJSON socket, event stream, every action) over the compositor's IPC; clients and AI agents never touch backend IPC, so the backend stays swappable (sway adapter possible later).
- Archetype workspaces routed by app-id/class rules + a spawn wrapper that injects distinguishing classes for same-binary archetypes (a Pi terminal and a plain terminal side by side).
- Maximimized-by-default window policy via backend rules: dialogs/transients float, per-app overrides, half/quarter snapping, no tiling tree.
- A Super-based keymap generated from the binding registry; the **bar's dropdown action menu and keybinding cheatsheet are generated from the same registry**, guaranteeing click/keyboard parity.
- One command palette (`lazydesk-menu`, our layer-shell client): fuzzy apps over a category tree, `>` WM verbs, `!` shell passthrough.
- Scratchpad via Ghostty's built-in quick terminal (zero custom code); idle screen-off without any lock screen; autospawned stack with `LAZYDESK_*` environment.
- Harness integration: `Super+a` focus-or-spawn Pi, shipped `lazydesk` agent skill, harness-driven app categorization, agent-editable hot-reloadable config, guaranteed agentless fallback mode.
- Consistent minimal theming from one token set: catppuccin-mocha default (matching the Pi harness theme), catppuccin-latte, and common dark alternatives — rendered by one Quickshell shell (bar, menu, notifications, OSD).
- **Packaging**: `lazydesk` install/update/doctor CLI (Void first-class, Debian/Ubuntu supported, no systemd assumption), dry-run mode, portable declarative user config — and install completeness: every shipped archetype must end up filled with a launchable app or be explicitly reported.
- **System controls** on established utilities: screenshots and region capture, screen recording, persistent clipboard history, volume/brightness keys with OSD, battery notifications — wired via generated binds and scripts, no custom daemons.

Out of scope for this change (follow-ups): the cockpit dashboard (`cockpit-dashboard` change), settings UI (`settings-ui` change), todos and quick notes (`notes-and-todos` change), agents/usage panels, crash-to-harness diagnosis, theme sync into apps, portals beyond the backend's own, a custom file manager (established managers reused behind the `files` archetype).

## Capabilities

### New Capabilities

- `window-management` — maximized-by-default placement, floating exception heuristic, per-app overrides, snapping, fullscreen, focus and close behavior.
- `workspaces` — archetype workspaces with matching rules, spawn-wrapper class tagging, ad-hoc workspaces, per-monitor sets, urgency.
- `keybindings` — Super mod, default binding table, user rebinding, config reload semantics.
- `command-palette` — single entry surface for app launching (fuzzy + categorized tree), WM verbs, and shell commands.
- `scratchpad` — global drop-down terminal toggled over any workspace.
- `session-startup` — autospawn of the app stack, environment, idle screen power-off, wake on input, session entry.
- `ipc` — LazyDesk-owned NDJSON socket protocol, `lazydeskctl` CLI, event stream, facade stability over the compositor backend.
- `harness-integration` — default harness focus-or-spawn, harness-driven app categorization, agent skill, agent-editable configuration, agentless fallback.
- `shell-ui` — bar (with action menu and keybinding cheatsheet), notifications, and one shared minimal theme with shipped presets.
- `packaging` — install/update CLI with archetype completeness guarantee, declarative portable config, managed generated artifacts, distro-agnostic packaging.
- `system-controls` — capture, clipboard history, volume/brightness OSD, battery awareness, built on established utilities.

### Modified Capabilities

(none — greenfield)

## Impact

- New monorepo: `lazydeskd` (facade daemon), `lazydeskctl` (CLI), `lazydesk-menu` (palette client), `lazydesk` (install/update CLI + generator), session scripts, agent skill, docs, theme packs, config templates; C23 + meson for binaries, POSIX shell where a script beats C.
- Runtime dependencies: Hyprland (backend), ghostty, quickshell (shell surfaces), Thunar (files archetype), plus small utilities; all packaged on Void/Debian/Ubuntu.
- The user's daily machine (Void) is the test bed; `foundation-maximized-shell` (own-compositor design) is retired from the plan — its behavior specs were carried into this change, only the foundation decision changed.
