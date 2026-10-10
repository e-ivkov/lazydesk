# Spec Delta

## Purpose

Defines what happens from session start to idle: autospawning the configured app stack, the environment apps see, and screen power behavior without any lock screen.

## ADDED Requirements

### Requirement: Autospawn the configured stack
On startup, once outputs are ready, LazyDesk SHALL spawn the configured startup entries in order, each tagged with its archetype workspace and per-entry launch command. An entry whose command cannot be executed SHALL be reported (notification + event) without aborting the remaining entries or startup.

#### Scenario: Normal login
- **WHEN** the user logs into a LazyDesk session with the default stack (pi terminal, Zed, Chromium, Telegram)
- **THEN** each app appears maximized on its archetype workspace without manual launching

#### Scenario: One app fails to start
- **WHEN** a configured entry references a missing binary
- **THEN** a notification reports it, the remaining entries still spawn, and startup completes

### Requirement: Session environment
Spawned processes SHALL inherit the user environment plus `LAZYDESK=1`, `LAZYDESK_SOCKET` (IPC socket path), and `LAZYDESK_ARCHETYPE` (the entry's archetype, if any). The session MUST set `XDG_CURRENT_DESKTOP=lazydesk` and `XDG_SESSION_TYPE=wayland` for itself and all children.

#### Scenario: Archetype visible to children
- **WHEN** the user inspects the environment of an autospawned Chromium process
- **THEN** `LAZYDESK_ROLE=web`, `LAZYDESK=1`, and a valid `LAZYDESK_SOCKET` are present

### Requirement: Idle screen power-off without locking
After the configured idle timeout (default 10 minutes), outputs SHALL power off. Any input event SHALL wake them. LazyDesk MUST NOT lock, blank-override, or require authentication on wake; no lock screen component SHALL be part of the session.

#### Scenario: Idle power-off
- **WHEN** no input occurs for the configured timeout
- **THEN** the screen goes dark and compositor rendering pauses

#### Scenario: Wake on keypress
- **WHEN** the user presses any key while screens are off
- **THEN** outputs power back on, the keystroke is delivered to the previously focused window, and no unlock prompt appears

### Requirement: Compositor restart resilience
If the compositor crashes, it MUST NOT take running clients with it needlessly: clients receive a defined disconnect, and on next login autospawn recreates the stack. Session state (config, palette history, scratchpad command) is persisted on change, not only at exit.

#### Scenario: Crash and relogin
- **WHEN** the compositor crashes during a session and the user logs in again
- **THEN** the configured stack respawns automatically with archetypes intact
