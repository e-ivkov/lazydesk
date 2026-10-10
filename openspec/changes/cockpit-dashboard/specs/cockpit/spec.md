# Spec Delta

## Purpose

Defines the cockpit dashboard: a DE-owned overview surface on the `cockpit` archetype workspace that reports system, device, WM, AI, and project/work state at a glance, read-only, with graceful agentless degradation.

## ADDED Requirements

### Requirement: Dashboard surface on the cockpit workspace
The cockpit SHALL be provided by the DE as a client running maximized on the `cockpit` archetype workspace, summoned by focus-or-spawn exactly like other archetypes (default binding `Super+c`), themed from the shared theme definition, and driven by IPC queries and events.

#### Scenario: Summon the dashboard
- **WHEN** the user presses `Super+c` while the editor workspace is focused
- **THEN** the cockpit workspace is focused with the dashboard maximized on it; pressing `Super+c` again from the editor returns to the editor workspace

### Requirement: System and devices overview
The cockpit SHALL display live system metrics — CPU, memory, swap, disk usage and I/O per mount, network I/O, and temperature/battery where the platform exposes them — plus a devices section listing outputs, input devices, and storage. Metrics MUST refresh at a configurable interval without user action, and device lists MUST reflect hotplug changes observed via events.

#### Scenario: Metrics update live
- **WHEN** the cockpit is visible and an interval elapses
- **THEN** CPU/memory/network values update without any user interaction

#### Scenario: Hotplugged output appears
- **WHEN** a second monitor is attached while the cockpit is visible
- **THEN** the devices section lists it after the next refresh

### Requirement: WM overview
The cockpit SHALL display current workspaces with window counts, per-window focus-time statistics, config health (last reload time and validation result), and IPC operation counters since session start.

#### Scenario: Config error visible
- **WHEN** a config reload fails and the user summons the cockpit
- **THEN** the WM section shows the failed reload with the validation error summary

### Requirement: AI overview with agentless badge
The cockpit SHALL display: whether a default harness is configured and running, the time and outcome of the last app categorization, and the age of cached AI summaries. When no harness is available, the section MUST show an explicit agentless badge and omit AI-derived content rather than showing errors or placeholders that imply action is pending.

#### Scenario: Agentless badge
- **WHEN** the cockpit opens on a machine with no harness installed
- **THEN** the AI section shows the agentless badge and the rest of the dashboard renders fully

### Requirement: Projects panel listing and ranking
The cockpit SHALL scan `~/projects` and present pinned projects (from configuration) plus a top-used list ranked by combined git activity recency and tracked window focus time.

#### Scenario: Pinned project always listed
- **WHEN** a project is pinned in configuration but has had no git activity for months
- **THEN** it remains at the top of the projects panel

### Requirement: Project entry content
Each project entry MUST show: name, primary languages by share of code, current branch, dirty state, and last commit summary; it MAY show a cached AI summary of recent edits generated offline via the harness invocation helper, cached with a configurable TTL, and degraded to plain git-stat text when agentless.

#### Scenario: Agentless project entry
- **WHEN** no harness is installed and a project entry is rendered
- **THEN** branch, dirty state, last commit, and language breakdown still display, with no AI summary line

### Requirement: Project entry actions
Entry actions MUST include: open in editor (editor archetype spawned at the project path), open in file manager (`files` archetype at the path), and copy path.

#### Scenario: Open project in editor
- **WHEN** the user activates "open in editor" on a project entry
- **THEN** the editor archetype's app spawns with the project directory as working directory, on the editor workspace

### Requirement: Work panel
The cockpit SHALL analyze `~/work` and present its top files ranked by recency and size, with a report of file-type/language distribution and total size, refreshed on demand and at a configurable interval; an optional cached AI summary of recent changes MUST degrade to a plain recency listing when agentless.

#### Scenario: Recently modified file surfaces
- **WHEN** a file in `~/work` was modified minutes ago and the cockpit refreshes
- **THEN** the file appears at or near the top of the work panel

### Requirement: Todos and notes panels
The cockpit SHALL display the open todos from the shared todo store and the most recent notes from the shared notes directory (see the notes-todos capability), with actions to complete/reopen todos and open notes in the editor archetype — all via public IPC verbs. These panels MUST function agentless and MUST reflect external edits to the underlying files without restart.

#### Scenario: Complete a todo from cockpit
- **WHEN** the user completes a todo on the cockpit panel
- **THEN** the corresponding `- [ ]` becomes `- [x]` in `todo.md` and the count updates

#### Scenario: Recently captured note listed
- **WHEN** a note was just captured from the palette and the cockpit refreshes
- **THEN** it appears at the top of the recent-notes panel

### Requirement: Read-only analysis with bounded cost
The cockpit MUST NOT modify anything under `~/projects` or `~/work`; its only writes are to its own cache. Scans MUST run asynchronously, be rate-limited, honor ignore patterns for large/irrelevant directories (e.g. `node_modules`, `.git` internals, build outputs), and never block dashboard interaction or the session while running.

#### Scenario: Huge repository scan
- **WHEN** `~/projects` contains a very large repository and the user interacts with the cockpit
- **THEN** the dashboard stays responsive and the scan completes or is skipped per its budget, never freezing the UI

#### Scenario: No side effects
- **WHEN** a full projects and work scan completes
- **THEN** `git status` in every scanned repository is unchanged and no file outside `~/.cache/lazydesk/` has a new mtime from cockpit

### Requirement: Actions use public IPC only
Every cockpit action MUST be expressed as public IPC operations (spawn, workspace focus) — no privileged or cockpit-only compositor path SHALL exist, so any agent or script can reproduce cockpit actions exactly.

#### Scenario: Action reproducibility
- **WHEN** an agent sends the same IPC operations the cockpit uses for "open in file manager"
- **THEN** the result is identical to triggering the action from the dashboard
