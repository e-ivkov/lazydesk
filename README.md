# LazyDesk

A development-first, packageable desktop profile on Hyprland — the Omarchy
pattern, tailored to one workflow: maximized-first archetype workspaces
(`agent editor terminal browser messenger cockpit lazygit` + `desk`/`files`),
one command palette (`Super+Enter`), Ghostty quick-terminal scratchpad, an
AI-harness (Pi) as a first-class citizen with a guaranteed agentless fallback.

- TOML under `~/.config/lazydesk/` is the single source of truth; a generator
  emits `hyprland.conf`, the Quickshell shell (bar, menu, notifications) and ghostty
  configs (marked, drift-checked).
- `lazydeskd` exposes LazyDesk's own NDJSON IPC; `lazydeskctl` is the CLI.
  Clients never touch backend IPC.
- Everything keybindable is clickable (bar action menu + cheatsheet generated
  from the same binding registry).

Design docs: `openspec/` (OpenSpec). Build: see `docs/build.md`.
Spdx: MIT.
