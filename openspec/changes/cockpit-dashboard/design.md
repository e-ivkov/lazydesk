# Design

## Context

Builds on change `lazydesk-core`: archetypes (workspaces spec), IPC + `lazydeskctl`, shared theme, harness invocation helper, and the `lazydesk-cockpit` binary slot in the monorepo. This design covers only what is novel: the dashboard client, its data pipeline, ranking, and caching.

## Goals / Non-Goals

**Goals:**

- One dashboard: system, devices, WM, AI, projects, work — readable at a glance, summoned in one keystroke.
- Read-only, bounded-cost analysis of `~/projects` and `~/work`.
- Agent-enhanced summaries that degrade cleanly to git-stat facts.
- Same visual language as bar/palette (cairo/pango, theme tokens).

**Non-Goals:**

- Process manager / task killer (htop territory).
- Editing projects, running builds/tests from the dashboard (that is the harness's job via IPC).
- Usage/subscription panels à la Omarchy (future change).

## Decisions

### D1: Regular maximized client, not a layer-shell overlay
Cockpit lives on its workspace like any app (xdg-toplevel, maximized). Summon = archetype focus-or-spawn, already specced. This keeps it out of the way (it is a destination, not an overlay like the scratchpad), lets it be closed/restarted freely, and reuses all window rules.

*Alternatives:* layer-shell overlay toggled over everything (wrong: hides context, fights with scratchpad semantics), separate monitor dashboard (non-goal for single-monitor laptop).

### D2: Rendering = cairo/pango like bar and palette
Same stack as `lazydesk-bar`/`lazydesk-menu`: no GTK, no toolkit deps, theme tokens shared. The dashboard is dense text/sections — perfect for pango layouts and simple boxes. One `lazydesk-cockpit` process; keyboard-driven section navigation (arrows/enter), minimal pointer support (click on entries/actions).

### D3: Data pipeline = collectors + cache, all async
A collector per section: `/proc` readers (CPU/mem/net/disk), device lists via IPC queries + udev-lite (`/sys` reads), WM stats via IPC events (it accumulates focus time by subscribing once at startup), git facts via `libgit2` (established C library — chosen over shelling `git` for speed and no-parse robustness), language breakdown via fast extension/size histogram honoring ignore rules (vendored `tokei`-like logic, count-only). Each collector runs on its own schedule, publishes into a shared snapshot guarded by a rwlock; rendering reads snapshots only. Budgets: scan time and file-count caps per project, configurable.

*Alternatives:* shell out to `git`/`tokei`/`iostat` per refresh (process storm on big trees), full SQL-ish indexer (overkill).

### D4: Ranking = decayed git activity + focus time
Top-used score: `w1 * exp(-age_of_last_commit/τ) + w2 * normalized_focus_seconds(30d)`. Focus time comes from IPC window-focus events accumulated in-session and persisted to `~/.cache/lazydesk/cockpit/focus.json` (project attribution = match window title/app to project name heuristically — documented approximation; cwd is not available for Wayland windows). Weights and τ configurable. Pinned projects bypass ranking.

### D5: AI summaries share the categorize pipeline
One helper (from foundation): `lazydeskctl ai run <prompt-file>` invoking the default harness non-interactively with a timeout, writing result + timestamp to `~/.cache/lazydesk/ai/`. Cockpit uses it for per-project "summary of last edits" (prompt: `git log --stat -20` + diffstat) and the work panel summary. TTL per artifact (default: 24h; project summary invalidated by new commits). Agentless: helper exits "no harness" instantly; collectors mark the section degraded. The DE never calls a model itself.

### D6: New IPC surface kept minimal
Foundation IPC already covers spawn/focus/queries. Added: `stats.focus` query (per-window focus seconds, persisted by compositor? — no: cockpit accumulates itself; compositor need only emit focus events, already specced) and `config.status` (last reload result — small addition to the config.reload event payload). Decision: extend the `config.reloaded` event with success/error detail rather than a new op; add `stats.ipc` op for counters (trivial, same dispatch table).

## Risks / Trade-offs

- [libgit2 dependency weight] → it's ubiquitous in distro repos, C-native, and removes the worst parsing surface; vendoring is possible but not planned.
- [Focus-time attribution is heuristic (title matching)] → documented approximation; ranking also works on git activity alone (weights configurable to zero).
- [Scan cost on huge trees] → budgets + ignore patterns + async; worst case is a stale section, never a stall.
- [Dashboard scope creep into a task manager] → non-goal enforced by "actions use public IPC only"; nothing cockpit can do that an agent cannot.

## Migration Plan

Purely additive; lands after foundation. No config migration — new keys (`[cockpit]` section, pins, TTLs, budgets) have safe defaults; absent keys = default behavior.

## Open Questions

- Section layout/order and keybindings inside the dashboard — usability call during implementation, no spec impact (spec names sections, not layouts).
- Whether `~/work` needs ignore-pattern defaults distinct from projects — reuse one list initially, split later if real usage demands.
