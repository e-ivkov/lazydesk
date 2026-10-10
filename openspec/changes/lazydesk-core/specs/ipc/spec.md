# Spec Delta

## Purpose

Defines the machine control plane of the DE: a LazyDesk-owned local socket protocol exposing every WM operation and a live event stream, consumed by `lazydeskctl`, the palette, the bar, scripts, and the AI harness — with the underlying compositor's IPC hidden as an implementation detail.

## ADDED Requirements

### Requirement: Every user action is an IPC operation
The DE SHALL expose a Unix-socket protocol at `$XDG_RUNTIME_DIR/lazydesk/ipc.sock` speaking newline-delimited JSON requests and responses. Every operation reachable by keybinding or palette verb SHALL be reachable by IPC, plus query operations: list outputs, workspaces, windows (with archetype, workspace, app id, geometry, and state). Actions are idempotent where meaningful.

#### Scenario: List windows from a script
- **WHEN** a client sends `{"op": "windows.list"}`
- **THEN** the response is a JSON array of all windows with their workspace, archetype, app id, and state

#### Scenario: Switch workspace from a script
- **WHEN** a client sends `{"op": "workspace.focus", "name": "agent"}`
- **THEN** the agent workspace is focused, identical to pressing `Super+1`

### Requirement: Event stream subscription
A client SHALL be able to subscribe to events (`window.map`, `window.unmap`, `window.focus`, `window.title`, `window.urgency`, `workspace.focus`, `workspace.urgency`, `config.reloaded`, `session.spawn-result`). Events are delivered as newline-delimited JSON on the same socket connection after a successful subscribe.

#### Scenario: Bar reacts to workspace change
- **WHEN** the user presses `Super+2` while a subscribed bar client is connected
- **THEN** the client receives `{"event": "workspace.focus", "workspace": 2, "name": "editor"}`

### Requirement: lazydeskctl CLI
The `lazydeskctl` binary SHALL wrap the protocol with a conventional CLI (`lazydeskctl <verb> [args]`, `lazydeskctl watch`, `lazydeskctl events`, JSON output mode for scripting) and exit non-zero with a readable error on protocol failures. It MUST be usable in pipelines by both scripts and AI agents without a TTY.

#### Scenario: Scriptable window query
- **WHEN** `lazydeskctl --json windows list` runs in a script
- **THEN** it prints machine-readable JSON and exits 0

#### Scenario: Error reporting
- **WHEN** `lazydeskctl snap bogus-direction` runs
- **THEN** it prints the protocol error message to stderr and exits non-zero

### Requirement: Spawn with archetype over IPC
IPC SHALL include a spawn operation with optional archetype and workspace arguments, used by autospawn, the palette, and external agents; where the app supports it the spawn injects a distinguishing app id/class (see workspaces spec).

#### Scenario: Agent spawns a terminal on a workspace
- **WHEN** `lazydeskctl spawn --archetype agent -- ghostty -e pi` runs
- **THEN** the process starts and its window lands on the agent workspace

### Requirement: Facade stability over the compositor
The LazyDesk protocol SHALL be the stable, documented control surface: clients MUST NOT need to know or use the underlying compositor's IPC. Replacing the compositor backend MUST remain possible without changing any client. Unknown operations MUST return a structured error, never crash the facade, and a backend failure MUST be reported as a structured error with the backend cause summarized.

#### Scenario: Malformed input tolerated
- **WHEN** a client sends a non-JSON line to the socket
- **THEN** the facade responds with a structured error and keeps serving other clients

#### Scenario: Backend quirk contained
- **WHEN** the compositor returns an error for an operation (e.g. focusing a nonexistent workspace)
- **THEN** the client receives a structured protocol error naming the operation, and the facade keeps serving

### Requirement: Protocol stability and access
The protocol SHALL be local-only (socket in the runtime dir, user-owned), versioned with a protocol version field queryable via IPC, and documented in the repository with request/response examples.

#### Scenario: Version query
- **WHEN** a client sends the version op
- **THEN** the response includes the protocol version, and the documented examples in `docs/ipc.md` run verbatim against it
