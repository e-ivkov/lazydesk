# Proposal

## Why

Config-as-API (TOML + hot reload) is perfect for agents and terminal people, but even a developer wants to see the whole configuration surface at once, record a keybinding by pressing it, and get instant validation feedback without remembering key names. A settings UI that edits the *same* files with schema awareness closes the loop — and being on the `desk` archetype workspace, it doubles as the home of DE self-management.

## What Changes

Introduces `lazydesk-settings`, a graphical settings client:

- Runs maximized on the `desk` archetype workspace (QtQuick app sharing the shell's theme QML module); summoned via `Super+comma`, palette verb `> settings`, or IPC; fully functional agentless.
- Edits every documented setting: archetype→app bindings (with per-archetype alternatives), autospawn entries, keybindings with press-to-record editing, window rules, scratchpad geometry, idle/outputs, palette categories tree, theme selection.
- Writes only the documented TOML files (no hidden state), validates before writing, applies via IPC reload, and surfaces reload errors inline.
- Detects external edits (agent or editor) while open and offers re-reading; never clobbers concurrent changes silently.

## Capabilities

### New Capabilities

- `settings-ui` — the configuration UI: coverage, validation-before-write, apply semantics, concurrent-edit handling.

### Modified Capabilities

(none — foundation's config-as-API and hot-reload requirements already define the semantics this UI consumes)

## Impact

- New binary `lazydesk-settings` in the monorepo (QtQuick/QML).
- Qt joins the stack as the DE's GUI toolkit family: settings shares the exact theme QML module the Quickshell shell renders from — one look, one import, zero CSS glue.
- Depends on change `lazydesk-core` (archetypes, IPC, config reload).
