# Proposal

## Why

Vibe-coding sessions generate todos ("remember to fix X") and fragments of knowledge that die in scrollback. A capture cost of under two seconds — without leaving the current window — plus plain-file storage the agent and editor can already read/write, plus recency-first surfaces in cockpit and a zoomable browser, makes the DE itself the notepad. Plain markdown keeps the data out of any app's silo forever.

## What Changes

Introduces the todos + notes layer:

- **Storage is plain markdown**: one file per note in `~/notes` (configurable), todos as a standard `- [ ]` task list in `~/notes/todo.md`; `#tags` anywhere. No database, no lock-in; git-friendly and directly editable by the harness.
- **Quick capture from the palette** (`Super+Enter`): `todo <text>` and `note <text>` commit instantly without leaving context; bare `todo`/`note` open list/quick-pick modes.
- **Quick capture from the bar menu**: "add todo" / "new note" entries open the same inline capture.
- **Todos lifecycle**: add, complete, reopen, delete — from capture surfaces, cockpit, the browser, or IPC verbs (`lazydeskctl todo …`), so agents manage the same list.
- **Zoomable notes browser** (`lazydesk-notes`, on the `desk` archetype): zoomed-out overview renders all notes' full content inline ordered by recency with grouping; zooming into a note focuses it for reading with edit handoff to the editor archetype; tag filtering narrows both levels.
- **Cockpit surfacing**: open todos and recent notes panels appear on the dashboard (see the cockpit-dashboard change).

## Capabilities

### New Capabilities

- `notes-todos` — storage format, quick-capture surfaces, todos lifecycle, notes browser with recency/tags/zoom, IPC verbs.

### Modified Capabilities

(none — cockpit panels are added inside the `cockpit-dashboard` change; palette and bar-menu integration build on lazydesk-core's existing verb and action-registry requirements)

## Impact

- New binary `lazydesk-notes` (cairo/pango client, same stack as cockpit) and capture verbs in the facade/CLI.
- Depends on `lazydesk-core` (palette verbs, action registry, IPC) and aligns with `cockpit-dashboard` (panels).
- Reads/writes only `~/notes` (configurable) — plain files, no migration ever needed.
