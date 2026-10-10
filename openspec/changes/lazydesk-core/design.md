# Design

## Context

Decision record: LazyDesk is built the Omarchy way — an opinionated, packageable DE profile on an established compositor — not a new compositor. The behavior specs from the retired `foundation-maximized-shell` change carried over nearly verbatim (they were written as behavior contracts); this design replaces the foundation. Reference points: Omarchy (config-glue DE on Hyprland: default agent, skill, installer, theming), sway IPC (facade inspiration), the user's Pi theme `catppuccin-mocha`.

## Goals / Non-Goals

**Goals:**

- The specced session on Niri in days, not months: archetypes, maximized-first, palette, scratchpad, autospawn, harness, theming.
- LazyDesk's IPC as the only sanctioned control surface — stable across backend changes.
- Packageable and reusable: `lazydesk install` on Void/Debian/Ubuntu; portable `~/.config/lazydesk/`.
- C23 for real code (facade, palette, generator core), shell where glue is glue.

**Non-Goals:**

- Writing or forking a compositor (retired; revisit only if the friction log ever proves need).
- Tiling trees, decorations, animations, lock screen, greeter, portals, settings GUI (follow-up change).
- Supporting every compositor on day one — one backend done well, adapter seam kept open.

## Decisions

### D1: Niri backend, chosen on packaging reality
Void (the daily driver) does not package Hyprland — only its libraries — so the Omarchy default dies on arrival here. Void ships niri fresh (26.04), and it covers every requirement natively: `open-maximized` window rules (maximized-first is one rule, not a hack), `open-on-workspace` app-id routing (archetype workspaces), named per-output workspaces, a documented JSON IPC with event stream (`niri msg`), layer-shell for Quickshell and our palette, and a `niri validate -c` command that gives us config verification in CI without a headless session. Debian/Ubuntu package niri in current releases. The facade (D3) keeps clients backend-agnostic: a Hyprland or sway adapter remains possible by writing another emitter + event normalizer. *Alternatives: Hyprland (rejected: unpackageable on Void without building from source — the exact anti-pattern this DE exists to avoid), sway (best IPC but no native maximize state), labwc/dwl (no usable IPC), own compositor (retired).*

### D2: TOML sources → generator → managed artifacts
`~/.config/lazydesk/lazydesk.toml` (+ `theme.toml`, `apps.toml`) is the single truth. The generator (`lazydesk config regen`, also run on reload) emits: `niri.kdl` (binds, window rules, workspaces, spawn-at-startup, environment), the Quickshell shell config + theme tokens (bar, menu, cheatsheet, notifications, OSD), a ghostty include (theme, quick terminal), and the palette theme file. Artifacts carry a `# lazydesk-managed` header; drift detection refuses silent overwrite (packaging spec). Hot reload = regenerate + backend reload + component restart signals, all orchestrated behind `lazydeskctl config reload` with a single `config.reloaded` result event.

### D3: `lazydeskd` facade daemon — LazyDesk IPC only
One Unix socket, NDJSON, versioned ops, normalized event stream (spec: ipc). Internals: translates ops to `niri msg` calls, normalizes niri event-stream events into the specced event vocabulary, implements what the backend can't do alone: `spawn --archetype` (D4), apps DB + categorize orchestration, config reload orchestration, focus-time accounting for cockpit. Crash-safe: systemd-less supervision is a simple respawn from the session script; clients reconnect and re-subscribe.

### D4: Archetype routing = class rules + spawn-wrapper injection
Config declares per-archetype match rules (app id/class). `lazydeskctl spawn --archetype X` launches via the facade, injecting a distinguishing class where the app supports it (Ghostty `--class=lazydesk-agent`), with `LAZYDESK_ARCHETYPE` in env. No process-lineage tracking — the one capability genuinely lost vs owning a compositor; class tagging covers the same-binary cases that mattered (two ghosttys) and everything else is plain rules. Documented tradeoff, accepted.

### D5: Scratchpad = Ghostty quick terminal
Ghostty ships a quake terminal (`global:toggle_quick_terminal`, configurable geometry/position/animation) — the entire scratchpad spec is satisfied by one generated config stanza + one generated Niri bind that forwards to `ghostty +toggle-quick-terminal`. No custom code; single instance and persistence are Ghostty's own guarantees.

### D6: `lazydesk-menu` palette is our own C23 layer-shell client
The palette grammar (three input modes, category tree, history, completion) is the DE's signature surface and no launcher has it; it's a client, compositor-agnostic, ~small cairo/pango app talking to the facade only. *Alternatives: walker/wofi custom modes (three tools where one surface is specced; worse, their config is theirs).*

### D7: Shell = Quickshell: bar, action menu, cheatsheet, notifications, OSD in one surface
Bindings live in TOML; the generator emits the Niri binds and the Quickshell shell config (bar with workspace indicators + urgency + harness indicator, the action menu listing every action with its combo, the keybinding cheatsheet overlay, notifications, DND, and the OSD) from the same registry. Parity by construction (old D14 survives): one source, three renderings. Click actions call `lazydeskctl` verbs — pointer users get everything keyboard users have. *This follows Omarchy v4, which replaced waybar+mako+walker with Quickshell for exactly these surfaces.* *Alternative rejected: waybar + mako (static JSON/CSS ceiling; menu module strains on full-registry menus).*

### D8: Idle via an idle-notify daemon (swayidle-class), dpms only, no locker ever (OSD/notification surfaces come from Quickshell)
Configured timeout → outputs off; wake on input. No lock component shipped, none wired — matches spec and user preference.

### D9: Themes: token TOML → all surfaces
`theme.toml` (palette, font, radius) + preset packs shipped: catppuccin-mocha (default — matches the user's Pi theme; official ghostty/zed ports exist so the desktop matches end-to-end), catppuccin-latte, gruvbox-dark, tokyo-night. Generator writes per-surface artifacts from tokens; switching = one key + reload. Theme switching via palette verb, settings (later), `lazydeskctl`.

### D10: Harness integration = Omarchy pattern, Pi-first, offline AI
`Super+a` focus-or-spawn (niri bind → facade op), `lazydeskctl apps categorize` invokes Pi non-interactively, validates, writes `apps.toml` (agentless: bundled default tree, clear error). The `lazydesk` skill documents IPC + TOML and is symlinked into `~/.pi/agent/skills` idempotently at session start. Agentless fallback per spec: nothing blocks, Super+a falls back to a terminal.

### D11: Packaging, session, and install completeness
`lazydesk` CLI (install/update/config/doctor, dry-run, drift checks) with per-distro dependency maps (xbps/apt). Archetype completeness is part of the contract: the maps cover every default archetype binding, install verifies each resolves to a launchable command, and doctor re-verifies on demand — an install that leaves an archetype unfilled has failed, loudly. Session entry: wayland-session desktop file → `lazydesk-session` script (env, socket dir, launches `niri -c` with the generated config; niri starts `lazydeskd`, the Quickshell shell, autospawn entries via spawn-at-startup). seatd and logind both supported; no systemd units required (optional user unit shipped for distros that want it). Install side-by-side safety: everything lives under LazyDesk prefixes or `~/.config/lazydesk/`; removing the package removes the DE, not the user's files.

### D12: System controls glue on established utilities
Screenshots (grim/slurp), recording (wf-recorder), clipboard (wl-clipboard + cliphist + a small selector overlay), audio (pactl), brightness (brightnessctl), OSD (rendered by our Quickshell shell), battery (upower queries + notifications): generated binds + thin scripts, zero new daemons, functional even if `lazydeskd` is down. Harvest policy: Omarchy is MIT and has accumulated exactly this glue (audio switching, capture, hw quirks like `omarchy-hw-nvidia-*`); cherry-pick scripts verbatim with license headers where they fit, rather than rewriting. Their hardware-quirk matrix is the one thing we point users at distro packages for in v1 and adopt opportunistically.

## Risks / Trade-offs

- [Niri config/IPC churn (fast-moving compositor)] → generator + facade isolate us: TOML and our protocol are stable, only the emitter/normalizer adapt; `niri validate` gates every generated config; pin tested versions per release.
- [Facade is a new always-on daemon] → tiny, supervised, restart-safe; worst case equals backend loss (session dies with compositor anyway).
- [Class injection unsupported by some apps] → plain match rules; worst case an app lands on the focused workspace (spec'd fallback), never lost.
- [Drift between generated artifacts and backend behavior changes] → doctor command validates round-trip (parse backend state via IPC, compare to TOML intent); CI smoke on pinned versions.
- [Ghostty quick-terminal geometry limits (fixed width/height per config, not dynamic)] → acceptable; spec allows configured geometry; revisit if a real need emerges.
- [Quickshell API churn across releases] → our shell config is generated and minimal (bar, menu, cheatsheet, notifications, OSD) and data-driven from the action registry; pin tested versions, update emitter per release.

## Migration Plan

Greenfield. Rollback = choose another session at login (stock niri/sway/etc. remain installed); LazyDesk artifacts are all marked-managed and removable via `lazydesk uninstall --keep-config`.

## Open Questions

- sway adapter priority — zero until someone asks; the seam is the facade, not a promise.
- Whether autospawn belongs to `lazydesk-session` or exec-once hooks — both work; decide during implementation (no spec impact).
