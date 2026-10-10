# Spec Delta

## Purpose

Defines the todos and quick-notes layer: plain-markdown storage under the notes directory, sub-two-second capture from the palette and bar menu, a recency-first zoomable browser, and scriptable todo/note operations shared with the AI harness.

## ADDED Requirements

### Requirement: Plain-markdown storage
Notes SHALL be stored as one markdown file per note in the configured notes directory (default `~/notes`); todos SHALL be stored as standard markdown task-list entries (`- [ ]` / `- [x]`) in the configured todo file (default `~/notes/todo.md`). Tags SHALL be `#hashtags` occurring anywhere in the text. External edits to these files (editor, scripts, agents) MUST be picked up by all DE surfaces without restart or reload commands.

#### Scenario: Agent edits the todo file
- **WHEN** the harness rewrites `~/notes/todo.md` adding a task
- **THEN** the next cockpit render and palette `todo` listing include it with no restart

#### Scenario: Plain files remain portable
- **WHEN** the user points a fresh machine's notes directory at a synced copy of `~/notes`
- **THEN** all notes, tags, and todo states are present with zero import steps

### Requirement: Quick capture from the palette
The palette SHALL support `todo <text>` committing a new todo and `note <text>` committing a new note (both accepting `#tags`), each completing without leaving the current window or workspace and confirming with a transient result row. Bare `todo` SHALL open a listing with complete/reopen/delete on entries; bare `note` SHALL open a recency-ranked quick-open of notes.

#### Scenario: Capture a todo mid-work
- **WHEN** the user presses `Super+Enter`, types `todo fix borrow checker error #lazywm`, presses `Enter`
- **THEN** the todo exists, the palette closes, and the previously focused window regains focus

#### Scenario: Capture a tagged note
- **WHEN** the user types `note wayland layershell reserves edges #graphics` in the palette
- **THEN** a new note file exists in the notes directory containing the text and tag

### Requirement: Quick capture from the bar menu
The bar action menu SHALL offer "add todo" and "new note" entries that open the same inline capture as the palette verbs.

#### Scenario: Capture from the menu
- **WHEN** the user clicks "new note" in the bar menu and types text
- **THEN** a note file is created exactly as via the palette path

### Requirement: Todos lifecycle
Todos SHALL support add, complete, reopen, and delete from: the palette listing, the cockpit todos panel, the notes browser, and IPC verbs. Completing a todo MUST rewrite its `- [ ]` marker to `- [x]` in the todo file (and vice versa for reopen) — no parallel state store exists.

#### Scenario: Complete from cockpit
- **WHEN** the user completes a todo on the cockpit panel
- **THEN** `grep` shows the corresponding `- [x]` line in `~/notes/todo.md`

### Requirement: Zoomable notes browser
The notes browser (`lazywm-notes`, opened via palette, verb, or bind, on the `lazywm` archetype workspace) SHALL have two zoom levels: an overview rendering every note's full content inline (not just titles) ordered by recency, and a focused single-note view for reading with an edit action that hands off to the editor archetype. Tag filtering SHALL narrow both levels; zoom transitions MUST NOT lose filter state.

#### Scenario: Zoomed out shows everything
- **WHEN** the user opens the browser and no filter is set
- **THEN** all notes are visible inline, most recently modified first

#### Scenario: Zoom in, filter, edit
- **WHEN** the user filters to `#graphics`, zooms into a note, and triggers edit
- **THEN** the editor archetype opens at that note's file and the filter persists when returning to overview

### Requirement: Recency handling
Default ordering everywhere (browser overview, palette quick-open, cockpit panel) SHALL be most-recently-modified first, with day-based grouping (today / yesterday / earlier) in the browser overview. A just-captured note or changed todo MUST appear at the top of every surface immediately.

#### Scenario: Fresh capture tops the list
- **WHEN** the user captures a note and opens the browser overview
- **THEN** the new note is the first card under "today"

### Requirement: Scriptable capture and queries
IPC/CLI verbs SHALL cover `todo add|done|reopen|rm|list` and `note add|list|show`, usable headlessly by agents and scripts with `--json` output.

#### Scenario: Agent works the todo list
- **WHEN** `lazywmctl todo add "review IPC docs" #lazywm && lazywmctl --json todo list` runs
- **THEN** the JSON list contains the new todo with its tag
