# Tasks

## 1. Toolchain and skeleton (P1)

- [x] 1.1 Install wlroots 0.20 + wayland/xkbcommon/libinput devel, wire a `wm/` meson target compiling a minimal wlroots program (display + autocreate backend + event loop), verify it runs nested (window under GNOME) and headless (WLR_BACKENDS) and exits cleanly
- [ ] 1.2 Add scene graph, renderer, xdg-shell: map a test client, show it, close it, verify nested via screenshot or quickshell bar presence
- [ ] 1.3 Implement maximize-on-map with usable-area computation (output minus layer-shell reserves) and keyboard focus, verify the regular-window scenario nested

## 2. Workspaces, keymap, IPC (P1)

- [ ] 2.1 Implement the workspaces engine (named archetypes from TOML via lazydesk-common, ad-hoc numbered, per-output, cleanup rule), verify the autospawn scenario nested
- [ ] 2.2 Implement keymap dispatch from the action registry (xkbcommon state, mods, pass-through of unbound Super grabs), verify each registry action fires and rebinds apply after reload
- [ ] 2.3 Move the NDJSON socket into the compositor event loop (version, windows/workspaces list, focus, move, spawn, reload ops + event subscription), verify `lazydeskctl` unchanged against it nested and headless
- [ ] 2.4 Implement spawn-with-archetype (app-id rules + class injection path parity), verify the two-ghostty routing scenario nested

## 3. Daily-drivable (P2)

- [ ] 3.1 Implement layer-shell + run the Quickshell bar on the native compositor, verify exclusive zones and workspace indicators live
- [ ] 3.2 Implement dialog/transient float with focus return, verify the file-picker scenario
- [ ] 3.3 Implement manual snap (halves, quarters, restore) via IPC + binds, verify the two-windows-side-by-side scenario
- [ ] 3.4 Implement urgency (xdg-activation), idle-notify + output power-off, multi-output hotplug with per-output workspace sets
- [ ] 3.5 Wire XWayland (satellite or built-in per design open question), verify an X client maps maximized

## 4. Graduation (P3)

- [ ] 4.1 Session entry flips to `lazydesk-wm` with supervisor respawn-once; niri stays selectable in emptty, verify rollback by choosing it
- [ ] 4.2 Crash test: kill -9 the compositor mid-session, verify respawn restores IPC + workspaces without relogin, second crash exits to greeter
- [ ] 4.3 CI: headless smoke suite runs the compositor + `lazydeskctl` round-trips, green on Void and one apt container
- [ ] 4.4 Daily-drive for a week on the native path; niri fallback exercised at least once; friction log triaged into follow-up changes

## Workflow follow-up

- Validate strictly; archive after 4.4.
- Follow-up: retire/keep decision for the niri emitter and `lazydeskd` bridge artifacts.
