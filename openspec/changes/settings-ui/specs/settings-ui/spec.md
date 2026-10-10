# Spec Delta

## Purpose

Defines the graphical configuration UI: full coverage of documented settings, validation before write, safe apply via hot reload, and correct handling of concurrent external edits — with TOML files remaining the single source of truth.

## ADDED Requirements

### Requirement: Settings surface on the lazydesk workspace
The settings UI SHALL be provided as a client running maximized on the `desk` archetype workspace, summoned via the configured binding (default `Super+comma`), a palette verb, or IPC, and MUST work identically with or without an AI harness present.

#### Scenario: Summon settings
- **WHEN** the user presses `Super+comma` from any workspace
- **THEN** the `desk` workspace is focused with the settings UI maximized on it

### Requirement: Full coverage of documented settings
The UI SHALL expose every setting documented in the configuration reference: archetype→app bindings (with the tested alternatives selectable), startup spawn entries, keybindings, window rules, scratchpad geometry and command, idle and output options, palette category tree, and theme choice. Any setting present in the files but unknown to the UI MUST be preserved untouched on write, and the UI MUST visibly list such unknown keys rather than hiding them.

#### Scenario: Unknown key preserved
- **WHEN** the config contains a key the settings UI has no widget for and the user applies unrelated changes
- **THEN** the unknown key survives byte-for-byte (comments excepted) and is listed in an "advanced/unknown" section

### Requirement: Validate before write, apply via reload
The UI MUST validate the complete resulting configuration before writing, reject invalid edits inline with the offending value and reason, and only then write the file and trigger reload via IPC. Reload errors from the compositor MUST surface in the UI; the previous-good-config semantics of the compositor guarantee the session never breaks.

#### Scenario: Binding recorded and applied
- **WHEN** the user records a new key combination for "spawn terminal", applies, and reload succeeds
- **THEN** the new binding works immediately with no restart, confirmed by the reload event

#### Scenario: Conflicting binding rejected
- **WHEN** the user records a combination already bound to another action and applies
- **THEN** the write is refused, the conflict is shown with both actions named, and the file is unchanged

### Requirement: Concurrent external edits detected
If a configuration file changes on disk while the UI has unsaved edits (e.g. an agent rewrote a binding), the UI MUST detect it before writing and offer a resolution: re-read external changes, keep local edits, or show a diff — it MUST NOT silently overwrite the external change.

#### Scenario: Agent edits while settings open
- **WHEN** the harness rewrites `lazydesk.toml` while the settings UI holds unsaved edits and the user clicks apply
- **THEN** the UI presents the conflict and writes nothing until the user resolves it

### Requirement: Single source of truth
The UI MUST read and write only the documented TOML files via their public schema; it MUST NOT maintain its own state store or settings database. Anything the UI can change, an agent or text editor can change with identical effect, by design.

#### Scenario: Edit equivalence
- **WHEN** a setting is changed in the UI and applied, versus the same change made by hand in the file and reloaded
- **THEN** the resulting compositor behavior is indistinguishable
