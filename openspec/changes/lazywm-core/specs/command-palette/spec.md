# Spec Delta

## Purpose

Defines the single command surface that replaces launcher, WM settings UI, and terminal one-off commands: the palette opened with `Super+Enter`.

## ADDED Requirements

### Requirement: One palette, three input modes
Pressing `Super+Enter` SHALL open a centered top-screen palette overlaying the current workspace without disturbing window state. Input is interpreted by prefix: bare text starts fuzzy app search, `>` starts a WM verb with completion, `!` runs a shell command. `Escape` closes the palette returning focus exactly as before.

#### Scenario: Open and dismiss
- **WHEN** the user presses `Super+Enter` and then `Escape`
- **THEN** the palette closes and the previously focused window regains keyboard focus unchanged

### Requirement: Fuzzy app search over a category tree
Bare input SHALL fuzzy-match applications from installed desktop entries, presented grouped under a category tree loaded from configuration. Selecting an entry launches the app on its configured archetype workspace, or on the focused workspace if the app has no archetype or is already running (existing window is focused instead of respawning).

#### Scenario: Launch known app
- **WHEN** the user types `z`, selects Zed, and presses `Enter`
- **THEN** Zed launches (or is focused if already running) maximized on the `editor` workspace

#### Scenario: Tree navigation
- **WHEN** the user types a category prefix such as `dev >` 
- **THEN** results are restricted to apps under that category subtree

### Requirement: WM verbs with completion
Input starting with `>` SHALL be parsed as a WM verb (the same operations exposed by IPC: workspace switching, moving, snapping, floating rules, spawning with an archetype, reloading config, quitting). The palette MUST offer completion and a short description per verb, and MUST reject unknown verbs with a visible error, executing nothing.

#### Scenario: Move window via verb
- **WHEN** the user types `> move 3` and presses `Enter`
- **THEN** the focused window moves to workspace 3 and the palette closes

#### Scenario: Unknown verb is a no-op
- **WHEN** the user types `> explod` and presses `Enter`
- **THEN** the palette shows "unknown verb" and performs no action

### Requirement: Shell passthrough
Input starting with `!` SHALL execute the remainder as a shell command in the user's login shell, inheriting the session environment, with output surfaced as a transient result row in the palette for the last command. The palette MUST NOT require confirmation for dev-shell one-liners.

#### Scenario: Quick command
- **WHEN** the user types `!cargo test -p foo` and presses `Enter`
- **THEN** the command runs in the background, its last output line appears as a result row, and the palette closes

### Requirement: History
The palette SHALL remember recent selections and shell commands, ranking fuzzy results by frequency and recency, persisted across sessions.

#### Scenario: Frequent app ranks first
- **WHEN** the user has launched Ghostty ten times and types `gh`
- **THEN** Ghostty ranks above less frequently used matches
