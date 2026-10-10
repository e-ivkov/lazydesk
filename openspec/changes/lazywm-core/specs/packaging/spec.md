# Spec Delta

## Purpose

Defines how LazyWM is shipped and installed as a packageable, reusable DE on top of its compositor backend: one install/update command, declarative user configuration, managed generated artifacts, and distro-agnostic packaging.

## ADDED Requirements

### Requirement: Install guarantees archetype completeness
After `lazywm install` finishes, every archetype in the shipped default configuration SHALL resolve to an installed, launchable app: the installer installs the per-distro package for each default binding (or an available configured alternative) and verifies the command resolves.

#### Scenario: Fresh install is complete
- **WHEN** `lazywm install` completes on a clean system with default configuration
- **THEN** all seven numbered archetypes plus `files` and the scratchpad terminal launch successfully on first login

### Requirement: Doctor reports unfilled archetypes
When a user configuration binds custom commands, install and `doctor` MUST verify each binding, report exactly which archetypes are unfilled with concrete fix suggestions, and exit non-zero until resolved — an install that would leave empty archetype workspaces must never finish silently.

#### Scenario: Custom binding reported
- **WHEN** `lazywm doctor` runs with `editor` bound to a custom binary that is not installed
- **THEN** it names the `editor` archetype, the missing command, and a suggested fix, and exits non-zero

### Requirement: Install and update via the lazywm CLI
LazyWM SHALL be installable and updatable through a single `lazywm` CLI (install, update, config regeneration, doctor) that works on Void, Debian, and Ubuntu: detects the distro, installs or verifies backend and component dependencies (compositor, terminal, bar, notification daemon, file manager), installs LazyWM's own binaries and data files, and registers the session entry. A dry-run mode MUST show planned actions before any change.

#### Scenario: Fresh install on Void
- **WHEN** `lazywm install` runs on a clean Void system
- **THEN** dependencies are installed via xbps, LazyWM files land in their prefixes, and the LazyWM session appears in the display-manager/greetd list

#### Scenario: Update is safe
- **WHEN** `lazywm update` runs while a session is active
- **THEN** binaries and data files are replaced, user configuration and generated-artifact markers are preserved, and the running session is not killed (changes apply on next login or reload)

### Requirement: User config is declarative and portable
All user configuration SHALL live under `~/.config/lazywm/` (TOML sources plus optional theme files). A fresh machine SHALL reach an equivalent session by copying that directory and running install. Files generated from these sources into backend/component locations SHALL be marked as LazyWM-managed and MUST NOT be silently overwritten if the user hand-edited them — the CLI detects the drift and asks.

#### Scenario: Portable setup
- **WHEN** the user copies `~/.config/lazywm/` to a new machine and runs `lazywm install`
- **THEN** after login the archetype workspaces, bindings, spawns, and theme match the original machine

#### Scenario: Hand-edited managed file detected
- **WHEN** the user hand-edits a generated backend config and later runs config regeneration
- **THEN** the CLI reports the drift and asks before overwriting

### Requirement: Distro-agnostic packaging
The project SHALL ship build/packaging metadata for its own components (meson-based) and documented dependency sets per target distro; nothing may hardcode one distro's paths or service managers. Seat/session launch MUST work under both seatd and logind setups.

#### Scenario: Debian/Ubuntu install
- **WHEN** `lazywm install` runs on Debian or Ubuntu
- **THEN** dependencies resolve via apt and the session entry works in greetd or a display manager

#### Scenario: No systemd assumption
- **WHEN** LazyWM runs on a seatd-based Void session
- **THEN** startup, IPC socket, idle handling, and autospawn all function without systemd user services
