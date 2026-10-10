# Spec Delta

## Purpose

Defines the global quake-style drop-down terminal that overlays any workspace for quick shell access without leaving context.

## ADDED Requirements

### Requirement: Global toggle over any workspace
`Super+grave` SHALL toggle a dedicated scratchpad terminal across the entire session: showing it raises it as an overlay on the current output regardless of which workspace is focused; hiding it withdraws it completely. The scratchpad MUST NOT appear in normal workspace focus cycling and MUST NOT take a workspace slot.

#### Scenario: Summon while coding
- **WHEN** Zed is focused on the `editor` workspace and the user presses `Super+grave`
- **THEN** the scratchpad terminal drops down over the `editor` workspace and takes keyboard focus

#### Scenario: Dismiss restores context
- **WHEN** the user presses `Super+grave` again
- **THEN** the scratchpad withdraws and Zed regains keyboard focus

### Requirement: Fixed overlay geometry and identity
The scratchpad SHALL occupy the full width of the output and the top portion defined in configuration (default 40% of height), rendered above maximized windows. Exactly one scratchpad instance SHALL exist per session; the toggle spawns it on first use if it is not yet running (or at session start when so configured).

#### Scenario: Single instance enforced
- **WHEN** the user presses `Super+grave` twice quickly
- **THEN** the same terminal instance toggles twice; no second scratchpad is ever spawned

### Requirement: Persistent state across toggles
The scratchpad terminal process MUST keep running while hidden: shell state, scrollback, and running processes survive toggle cycles. Closing the terminal window from inside (e.g. `exit`) SHALL simply end that instance, and the next toggle spawns a fresh one.

#### Scenario: Background job survives
- **WHEN** the user starts `cargo build` in the scratchpad, hides it, works in Zed, and re-summons it
- **THEN** the build output and prompt are exactly as left

#### Scenario: Exit inside scratchpad
- **WHEN** the user types `exit` in the scratchpad shell
- **THEN** the overlay disappears and the next `Super+grave` spawns a new scratchpad terminal

### Requirement: Configurable terminal command
The command used for the scratchpad SHALL be configurable (default: the configured terminal running a shell), independent of the harness archetype terminal.

#### Scenario: Different terminal
- **WHEN** configuration sets `scratchpad.command` to another terminal binary
- **THEN** the next freshly spawned scratchpad uses it, while any running instance is left untouched
