# Spec Delta

## Purpose

Defines the keyboard control surface: Super as the modifier, the default binding table, user rebinding via configuration, and config reload.

## ADDED Requirements

### Requirement: Super is the modifier
All LazyWM bindings SHALL use Super (Mod4) as the primary modifier. LazyWM MUST NOT intercept keys when Super is not involved, and MUST pass unrecognized Super combinations through to the focused client when the focused app has grabbed them (e.g. editor chords).

#### Scenario: Plain typing unaffected
- **WHEN** a user types in Ghostty without Super held
- **THEN** every keystroke reaches Ghostty unmodified

#### Scenario: Unused Super chord passes through
- **WHEN** the focused app has registered a Super-based shortcut not bound in LazyWM
- **THEN** the app receives the key event

### Requirement: Default workspace and window bindings
LazyWM SHALL ship these defaults, every entry rebindable:

- `Super+1..9,0` switch to workspace N; `Super+Shift+…` move window there
- `Super+n` new ad-hoc workspace
- `Super+j` / `Super+k` cycle focus next/previous
- `Super+Ctrl+arrows` snap to halves (quarters via two-step combo); `Super+Ctrl+Up` restore maximized
- `Super+f` toggle fullscreen; `Super+q` close window

#### Scenario: Switch to archetype workspace
- **WHEN** the user presses `Super+4`
- **THEN** the `browser` workspace becomes focused on the current output and its topmost window takes keyboard focus

### Requirement: Default launch and command bindings
LazyWM SHALL ship these defaults, every entry rebindable:

- `Super+Enter` command palette
- `Super+grave` toggle scratchpad terminal
- `Super+a` focus-or-spawn default harness (agentless fallback: terminal)
- `Super+Backspace` terminal on the focused workspace (`terminal` archetype command)
- `Super+c` cockpit; `Super+comma` settings (`lazywm` workspace); `Super+e` file manager at `~/projects`
- `Super+Escape` reload config; `Super+Shift+Escape` quit with confirmation

#### Scenario: Quick terminal anywhere
- **WHEN** the user presses `Super+Backspace` on any workspace
- **THEN** a terminal window opens maximized on that workspace using the `terminal` archetype's configured command

#### Scenario: Quit confirmation
- **WHEN** the user presses `Super+Shift+Escape`
- **THEN** the palette opens with a quit prompt and LazyWM exits only after confirmation

### Requirement: User rebinding
The configuration MUST allow rebinding every action, adding bindings for any IPC verb, and removing default bindings. A configuration containing a binding conflict or unknown action MUST be rejected at load with the offending entry reported, keeping the previous bindings active.

#### Scenario: Custom binding for a verb
- **WHEN** configuration maps `Super+Shift+t` to the `spawn` verb with a command
- **THEN** pressing it spawns that command on the focused workspace

#### Scenario: Invalid binding rejected
- **WHEN** the user reloads configuration that binds `Super+j` twice
- **THEN** reload fails with a diagnostic naming the conflict and the previously loaded bindings remain in effect

### Requirement: Configuration reload
Reloading configuration (via `Super+Escape` or IPC) SHALL apply new bindings, rules, and theme values without restarting the compositor or disturbing mapped windows.

#### Scenario: Hot reload during work
- **WHEN** the user edits a per-app float rule and reloads
- **THEN** already-mapped matching windows are re-evaluated against the new rule and mapped windows stay on their workspaces
