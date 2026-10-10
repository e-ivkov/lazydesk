# Spec Delta

## Purpose

Makes the AI coding harness a first-class session citizen, Omarchy-style but Pi-first: summonable, able to drive the DE over IPC, responsible for app categorization, and the author of its own desktop configuration.

## ADDED Requirements

### Requirement: Default harness focus-or-spawn
Configuration SHALL name a default harness (default: `pi` inside the configured terminal on the `agent` workspace). `Super+a` SHALL focus the harness's `agent` workspace and its existing window if one exists; otherwise it SHALL spawn the harness command. The harness MUST be treated as an app, not a privileged component.

#### Scenario: Summon after login
- **WHEN** the user presses `Super+a` on a fresh session where the autospawned pi terminal is already running
- **THEN** the pi workspace is focused and the harness terminal gets keyboard focus

#### Scenario: Harness not running
- **WHEN** the harness window was closed and the user presses `Super+a`
- **THEN** the harness command is spawned on the pi workspace

### Requirement: The DE is harness-controllable
Because every action is an IPC operation, the running harness MUST be able to fully observe and drive the session through `lazywmctl` (list windows, move focus, spawn, reconfigure), using only its normal shell access. No harness-specific privilege path SHALL exist.

#### Scenario: Agent rearranges the session
- **WHEN** the user asks the harness to "open the browser next to chat"
- **THEN** the agent runs `lazywmctl` verbs that focus `browser`, snap windows as asked, and the desktop reflects it

### Requirement: Shipped lazywm agent skill
The repository SHALL ship an agent skill (`lazywm`) that documents the IPC, configuration format, keybindings, and safe reconfiguration procedure, installable into the default harness's skill directory at session setup (idempotent, never overwriting user edits).

#### Scenario: Skill available to Pi
- **WHEN** a session starts and Pi is the default harness
- **THEN** `lazywm` appears among the agent's skills with current IPC documentation

### Requirement: Harness-driven app categorization
The palette's category tree SHALL be regenerable by the default harness: a `lazywmctl apps categorize` command composes a prompt (installed desktop entries + current tree) and invokes the default harness non-interactively to produce a new tree, which is written to configuration only after validation (valid categories, every entry still reachable). Manual edits and manual lock flags MUST survive regeneration.

#### Scenario: First categorization
- **WHEN** `lazywmctl apps categorize` runs on a fresh install
- **THEN** the harness proposes a tree, it is validated and saved, and the palette immediately groups apps by it

#### Scenario: Bad proposal rejected
- **WHEN** the harness produces a tree that omits reachable apps or is malformed
- **THEN** configuration is left unchanged and the command exits non-zero with the validation error

### Requirement: Agentless fallback operation
The DE MUST remain fully functional with no harness installed: archetypes resolve to non-agent apps, palette fuzzy search uses a bundled default category tree, AI-derived summaries are omitted rather than blocking, and `apps categorize` reports unavailability clearly. Agent-dependent affordances degrade visibly instead of failing silently. Agent-present operation remains the primary mode; agentless is a supported fallback.

#### Scenario: Boot without any harness
- **WHEN** the session starts on a machine with no coding-agent CLI installed
- **THEN** autospawn, workspaces, palette, scratchpad, cockpit, and settings all function, with AI-derived features visibly marked unavailable

#### Scenario: Categorize without a harness
- **WHEN** `lazywmctl apps categorize` runs with no harness configured
- **THEN** it exits non-zero with an explanatory message and configuration is untouched

### Requirement: Harness summon has an agentless fallback
`Super+a` MUST NOT become a dead key when no harness is configured: it SHALL spawn the configured fallback action (default: a terminal on the `agent` workspace).

#### Scenario: Summon without a harness
- **WHEN** the user presses `Super+a` with no harness installed
- **THEN** a terminal window opens maximized on the `agent` workspace

### Requirement: Configuration as the agent API
All session behavior (archetypes, bindings, rules, spawn entries, theme values) SHALL live in hot-reloadable TOML files under the standard config directory. An agent editing and reloading these files MUST achieve the same results as any built-in settings path; there is no second, hidden store.

#### Scenario: Agent changes a binding
- **WHEN** the harness rewrites a binding in `lazywm.toml` and calls `lazywmctl config reload`
- **THEN** the reload event fires and the new binding works, with no restart
