# Spec Delta

## Purpose

Defines the workspace model: archetype-named workspaces (archetypes are abstract roles mapped to concrete apps via matching rules), numbered ad-hoc workspaces, and how they map to one or more monitors.

## ADDED Requirements

### Requirement: Archetype workspaces routed by matching rules
The configuration SHALL define an ordered set of archetype workspaces. Shipped defaults: `1 agent`, `2 editor`, `3 terminal`, `4 browser`, `5 messenger`, `6 cockpit`, `7 lazygit`. Each archetype declares matching rules (app id / window class, with optional title patterns); windows matching an archetype's rules SHALL be placed on that archetype's workspace. Windows matching no rules land on the focused workspace.

#### Scenario: Autospawned stack lands on archetypes
- **WHEN** the session autospawns the configured apps and their first windows appear
- **THEN** each lands on the archetype workspace whose rules match its app id or class

#### Scenario: Unknown app stays local
- **WHEN** an app with no matching rule opens a window
- **THEN** the window opens maximized on the focused workspace

### Requirement: Spawn wrapper tags same-binary archetypes
The DE's spawn operation (`lazydeskctl spawn --archetype <name>`) SHALL, for apps that support it, launch the command with an archetype-distinguishing app id/class (e.g. Ghostty's `--class`), so multiple instances of the same binary can be bound to different archetypes (a Pi terminal and a plain terminal side by side). Where an app cannot be tagged, plain matching rules SHALL be used.

#### Scenario: Two terminals, two archetypes
- **WHEN** the session spawns a Pi harness terminal on `agent` and a plain terminal on `terminal`, both Ghostty
- **THEN** their windows land on the correct workspaces via their distinct injected classes

### Requirement: Archetypes are app bindings, not app identities
Workspace semantics MUST depend only on the archetype, never on the concrete app bound to it. Swapping an archetype's app binding MUST require no other configuration change: windows of the newly bound app route to the archetype workspace exactly as before, and already-running windows are unaffected.

#### Scenario: Editor swapped to a terminal-based editor
- **WHEN** configuration rebinds `editor` from Zed to `ghostty -e nvim` and the user launches the editor
- **THEN** the nvim window opens maximized on the `editor` workspace and focus cycling treats it identically

#### Scenario: Browser swapped
- **WHEN** configuration rebinds `browser` from Chromium to Firefox
- **THEN** Firefox windows land on the `browser` workspace with no other change needed

#### Scenario: Alternative bindings are first-class
- **WHEN** the shipped defaults are tested with sane alternative bindings per core archetype (editor: nvim-in-terminal, browser: Firefox, terminal: a second terminal, messenger: a second client)
- **THEN** all workspace, focus, urgency, and palette behaviors pass identically for each alternative

### Requirement: Cockpit, lazygit, desk, and files archetypes
Beyond the app-replacement archetypes, the configuration SHALL include: `cockpit` (dashboard component, see the cockpit-dashboard change), `lazygit` (terminal git client), `desk` (the DE's own surfaces: settings and future DE tools), and `files` (file manager). `desk` and `files` ship unnumbered by default but MUST be assignable a workspace number like any other archetype.

#### Scenario: Lazygit one keystroke away
- **WHEN** the user presses `Super+7`
- **THEN** the `lazygit` workspace is focused with the terminal git client maximized on it

#### Scenario: File manager on its own workspace
- **WHEN** configuration gives `files` the number 8 and the user opens the file manager
- **THEN** it lands maximized on workspace 8 and `Super+8` focuses it

### Requirement: Ad-hoc workspaces
The user SHALL be able to create additional unnamed numbered workspaces on demand (via keybinding, palette, or IPC), move windows to any workspace, and switch to any workspace by number. Switching MUST change only the active workspace of the affected output.

#### Scenario: Create a scratch workspace
- **WHEN** the user triggers "new workspace" and then spawns a terminal from the palette without an archetype
- **THEN** the terminal opens maximized on the new workspace

#### Scenario: Move window to archetype workspace
- **WHEN** the user moves the focused window to the `browser` workspace
- **THEN** the window disappears from the current workspace and reappears maximized on `browser`

### Requirement: Multi-monitor support
Each output SHALL have its own independent set of workspaces. Archetype workspaces live on the output designated in configuration (default: first output). The user SHALL be able to focus outputs and move workspaces between outputs via IPC and keybindings. With a single monitor configured, behavior MUST be identical to the single-output model.

#### Scenario: Laptop single screen
- **WHEN** LazyDesk runs with one output
- **THEN** all archetype and ad-hoc workspaces live on that output and `Super+number` cycles among them

#### Scenario: Second monitor attached
- **WHEN** a second output is hotplugged
- **THEN** it gains its own empty workspace set, focus can be moved to it, and existing workspaces keep their positions

### Requirement: Workspace urgency
A workspace SHALL surface an urgency hint (via bar, event stream, and notification policy) when a window on it requests activation or attention while the workspace is not focused. Switching to the workspace MUST clear the hint.

#### Scenario: Messenger message on unfocused workspace
- **WHEN** a Telegram window on the `messenger` workspace requests attention while `agent` is focused
- **THEN** the `messenger` workspace indicator shows urgency in the bar and the event stream carries a workspace urgency event

### Requirement: Empty ad-hoc workspace cleanup
An ad-hoc workspace with no windows SHALL be removed automatically once it is no longer focused. Archetype workspaces MUST never be removed.

#### Scenario: Last window closed
- **WHEN** the only window on ad-hoc workspace 9 is closed
- **THEN** workspace 9 disappears and focus falls back to the most recently used workspace
