# Design

## Context

Builds on change `lazydesk-core` (TOML config-as-API, hot reload with previous-good semantics, IPC, archetypes, theme tokens). The only open question this design answers: how to get a real settings UI without abandoning the single-source-of-truth principle or dragging the whole DE into a toolkit.

## Goals / Non-Goals

**Goals:**

- Every documented setting editable, with schema-aware widgets and instant validation.
- TOML files remain the only store; UI is a fancy editor, not a parallel database.
- Safe under concurrent writers (user, agent, text editor).
- Works agentless; no hidden state.

**Non-Goals:**

- Live-editing every compositor internal without reload (reload is the apply mechanism — it is already hot).
- A control-center for network/Bluetooth/etc. (system settings belong to the distro's tools; LazyDesk settings are LazyDesk's own surface).

## Decisions

### D1: QtQuick (QML) app sharing the shell's theme module — one Qt family
Forms, toggles, lists, key-recording, dialogs: hand-rolling these in cairo would be weeks of NIH for a worse result. QtQuick is established, packaged on Void/Debian/Ubuntu, and — decisive — the Quickshell shell already renders our bar/menu/notifications from a generated theme QML module; `lazydesk-settings` imports the very same module, so settings and shell share one theme, one look, one reload path. Scope discipline: QML lives in `lazydesk-settings` + the generated Quickshell config; palette and cockpit stay cairo (text-simple surfaces).

*Alternatives:* GTK4 (rejected: drags in a second GUI toolkit just for one app), cairo-only (rejected: form-widget NIH), Qt Widgets (viable, but QML shares the shell's stack and theming), TUI (rejected: intentionally the GUI surface), web-tech (rejected outright).

### D2: Load → model → validate → serialize → reload
The UI parses all config files into a schema-aware model. Apply = validate model → serialize to TOML (preserving unknown keys verbatim via a comment-preserving TOML editor: a small CST-preserving writer we control) → write atomically (tmp+rename) → `lazydeskctl config reload` (which regenerates the managed backend/component artifacts and reloads them) → listen for the `config.reloaded` event (success/error detail as added by cockpit-dashboard D6). Errors render inline per field plus the facade's error text when reload itself rejects.

### D3: Conflict detection by mtime+hash
Track (mtime, size, content hash) per file at load and before every write; on mismatch with unsaved local edits → three-way resolution dialog (external / local / diff view) implementing the concurrent-edits requirement. Agents editing files are a *primary* flow here, not an edge case.

### D4: Keybinding editor records real key combos
A grab item captures the next pressed combination (modifiers + key) through QtQuick's Keys handling, converts to LazyDesk's binding syntax (same tokens as TOML and palette verbs — one grammar), and checks conflicts against the full binding set client-side before submit.

### D5: Coverage contract
A machine-readable settings schema (`docs/config-schema.toml`: key, type, range, default, widget hint, tested alternatives per archetype) drives both the reference docs and the UI's widget choice. If a key isn't in the schema, the UI shows it in the advanced/unknown list and preserves it on write. This keeps spec requirement "full coverage" testable: CI asserts every documented key has a widget mapping.

## Risks / Trade-offs

- [QML/Qt version drift across distros] → QML from the Qt 6 series, no exotic modules; theme module is generated data (tokens), not code.
- [CST-preserving TOML writer is custom code] → limited scope: preserve unknown keys + top-level comments; formatting of untouched nodes must be byte-stable (property-tested against fixtures).
- [Two writers racing (UI apply vs agent reload)] → atomic rename + mtime/hash detection + reload event ack; worst case is a rejected apply with a diff shown, never corruption.
- [Scope creep toward distro control center] → non-goal pinned; LazyDesk settings only.

## Migration Plan

Additive; lands after foundation. No existing keys change. Missing `docs/config-schema.toml` keys simply render as unknown-key rows until documented.

## Open Questions

- None blocking; widget-level layout is implementation detail.
