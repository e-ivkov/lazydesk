# Design

## Context

Builds on `lazydesk-core` (palette verbs, action registry, facade IPC, `desk` archetype) and aligns with `cockpit-dashboard` (todos/notes panels are specced there). The data model is deliberately boring: plain markdown, because the primary co-editor is an AI harness and a human with nvim, not a database.

## Goals / Non-Goals

**Goals:**

- Sub-two-second capture without context switch, from palette and bar menu.
- One source of truth: the files. Every surface is a renderer over `~/notes`.
- Zoomable reading surface for notes; actionable todo list everywhere it matters (cockpit, palette, browser, CLI).
- Agent parity from day one (same verbs, same files).

**Non-Goals:**

- Sync/backends (the directory is the interface — sync it however), rich-text, encrypted notes, kanban, calendar/due dates, subtasks.
- A separate todo app or database — one markdown file.

## Decisions

### D1: Storage = one file per note + one todo.md
`~/notes/<yyyymmdd-hhmmss>-<slug>.md` (timestamped names sort naturally and never collide) and `~/notes/todo.md` with standard `- [ ]` checkboxes. Tags are inline `#hashtags` — no frontmatter needed (zero ceremony for one-line notes; frontmatter allowed, ignored). Invalidation: cheap directory rescan on each surface open + mtime short-circuit; inotify later if it ever matters. *Alternatives: frontmatter index/DB (a second thing to corrupt), org-mode (ecosystem yes, but markdown is the lingua franca of agents and renderers).*

### D2: Capture flows through the facade
Palette and bar-menu capture call `lazydeskctl todo/note add` (facade op) — so the write path, file naming, and "capture happened" event are identical for humans, menu clicks, keybinds, and agents. Palette closes on commit with a one-line confirmation; no focus theft (already specced).

### D3: Browser = cairo client, two zoom levels + filter
`lazydesk-notes` on the `desk` archetype, same stack as cockpit. Overview: pango-laid note cards in a vertical flow, full content inline, day-grouped headers, tag filter row; focused view: single note large with edit/delete/tag actions. Edit hands off to the editor archetype at the file path (`lazydeskctl spawn --archetype editor -- $EDITOR_OPEN_FMT path`), returning to overview keeps filter. Keyboard-first navigation (j/k/enter/esc), pointer works too.

### D4: Todos are lines, states are markers
Complete/reopen = line rewrite in `todo.md` (atomic tmp+rename, tolerant of concurrent editors: match on full line text, last write wins, re-read on conflict). Sections (e.g. `## later`) allowed and preserved — everything that isn't a task line passes through untouched.

## Risks / Trade-offs

- [Concurrent edit races (agent + user)] → atomic rewrites + full-line matching + immediate re-read; worst case a stale checkbox toggle, never file corruption.
- [One big todo.md at scale (years)] → sections + archive verb (`todo archive` moves done lines to `todo-archive.md`) — keep v1 simple, archive is a trivial follow-up.
- [Full-content overview with hundreds of notes] → render windowed (virtual flow), day groups collapse older-than-threshold; KISS until it hurts.

## Migration Plan

Additive; depends on lazydesk-core. If `~/notes` exists with files, they are simply notes from day one (untimestamped names keep their mtime ordering).

## Open Questions

- None blocking; browser layout details are implementation-time.
