# Tasks

## 1. Repository and packaging scaffold

- [ ] 1.1 Create the monorepo layout (`daemon/`, `ctl/`, `menu/`, `tools/lazywm` CLI, `session/`, `themes/`, `skill/lazywm`, `docs/`, config templates) with meson building a C23 hello for each binary, verify `meson setup build && ninja -C build` succeeds
- [ ] 1.2 Vendor tomlc and the JSON library, verify round-trip unit tests pass
- [ ] 1.3 Implement the `lazywm` CLI skeleton (install/update/config regen/doctor, dry-run, per-distro dependency maps for Void/Debian/Ubuntu), verify dry-run on all three distros prints a correct plan (container or VM)

## 2. Config model and generator

- [ ] 2.1 Implement TOML config model (archetypes, bindings, rules, spawns, scratchpad, harness, idle, outputs, theme, categories) with validation and previous-good semantics, verify an invalid edit is rejected with the offending key named
- [ ] 2.2 Implement the generator emitting `hyprland.conf` (bindings, windowrules, workspaces, exec-once, env) with `# lazywm-managed` markers, verify a headless Hyprland loads the generated config with zero warnings
- [ ] 2.3 Emit waybar config+CSS, mako config, and ghostty include from the same model (incl. theme tokens and quick-terminal stanza), verify each component starts with its generated file
- [ ] 2.4 Implement drift detection for managed artifacts, verify the hand-edited-managed-file scenario (refuses silent overwrite, reports drift)

## 3. Session and window policy

- [ ] 3.1 Implement `lazywm-session` (env export, socket dir, Hyprland launch with generated config, component startup incl. `lazywmd` and waybar), verify login reaches a desktop with bar on a test machine
- [ ] 3.2 Implement maximized-by-default policy via generated windowrules (dialogs/transients float, per-app overrides), verify the file-picker, forced-maximize, and second-window scenarios from the window-management spec
- [ ] 3.3 Implement archetype routing rules + spawn-wrapper class injection (two-ghostty scenario), verify every workspaces-spec scenario incl. alternative bindings (editor→nvim, browser→firefox, terminal→foot)
- [ ] 3.4 Implement snapping binds (halves, quarters, restore), fullscreen toggle, focus cycling, urgency passthrough, verify against the window-management and workspaces scenarios
- [ ] 3.5 Implement idle via hypridle (dpms off, wake on input, no locker), verify both idle scenarios

## 4. Facade IPC and lazywmctl

- [ ] 4.1 Implement `lazywmd`: NDJSON socket, op dispatch, version op, structured errors, backend translation to Hyprland, event normalization into the specced vocabulary, verify malformed-input and backend-quirk scenarios
- [ ] 4.2 Implement query ops (outputs/workspaces/windows) and action ops (focus, move, snap, float, spawn, reload, quit), verify each op against its spec scenario via a test client
- [ ] 4.3 Implement event subscription incl. `config.reloaded` with success/error detail, verify the bar-style subscriber receives workspace.focus on `Super+2`
- [ ] 4.4 Implement `lazywmctl` (verbs, `watch`, `--json`, exit codes, spawn with archetype), verify scriptable-query and error-reporting scenarios and `docs/ipc.md` examples running verbatim

## 5. Keymap, bar, palette

- [ ] 5.1 Implement the default binding table through the generator (see keybindings spec incl. Super+Backspace/c/comma/e), verify each binding triggers its action on a live session
- [ ] 5.2 Generate the waybar action menu and keybinding cheatsheet from the binding registry, verify menu-item-equals-keybinding, rebound-key-reflected, and cheatsheet-reflects-rebinds scenarios
- [ ] 5.3 Implement `lazywm-menu` palette (layer-shell, three input modes, fuzzy+tree+history, verb completion, shell passthrough), verify all command-palette spec scenarios
- [ ] 5.4 Implement Super-passthrough behavior: unbound Super combos reach apps that grabbed them

## 6. Scratchpad, autospawn, themes

- [ ] 6.1 Wire the scratchpad to Ghostty quick terminal via generated config + bind, verify toggle-over-anything, persistent-state, exit-and-respawn, and configurable-command scenarios
- [ ] 6.2 Implement autospawn entries (env vars `LAZYWM*`, failure notification without blocking), verify normal-login and failing-entry scenarios
- [ ] 6.3 Ship theme packs (catppuccin-mocha default, catppuccin-latte, gruvbox-dark, tokyo-night) with generator output for bar/palette/mako/ghostty, verify restyle-on-reload and light-theme-switch scenarios

## 7. Harness integration

- [ ] 7.1 Implement `Super+a` focus-or-spawn with agentless terminal fallback, verify both summon scenarios and the fallback scenario
- [ ] 7.2 Ship the `lazywm` agent skill (IPC reference, TOML format, safe reconfiguration) with idempotent session install into `~/.pi/agent/skills`, verify skill presence and no clobber of user edits
- [ ] 7.3 Implement `lazywmctl apps categorize` (prompt compose, Pi non-interactive, validation, locks preserved, bundled default tree fallback), verify accept, reject, and agentless scenarios
- [ ] 7.4 Verify the agent-rearranges-session scenario end-to-end: Pi moves windows/workspaces via `lazywmctl` only

## 8. Integration, packaging, agentless proof

- [ ] 8.1 Wire headless smoke CI (Hyprland headless + `lazywmctl` assertions) covering one scenario per capability, green on Void and Debian/Ubuntu containers
- [ ] 8.2 Verify `lazywm install` end-to-end on clean Void and one apt distro (session entry in greetd/DM list, portable-config scenario by copying `~/.config/lazywm/` to a second machine/container)
- [ ] 8.3 Verify full agentless boot: no harness installed, all functions work, AI features visibly degraded, config untouched by categorize
- [ ] 8.4 Daily-drive the session for one week on the Void machine, keep the friction log, fix P0s, file follow-up proposals for the rest

## Workflow follow-up

- Review change against specs: `openspec validate --strict`
- Archive the change via OpenSpec once reviewed and scenarios verified.
- Then implement `cockpit-dashboard`, then `settings-ui`.
