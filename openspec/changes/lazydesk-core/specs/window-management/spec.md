# Spec Delta

## Purpose

Defines how every window is placed on screen: maximized by default, floating only for dialog-shaped windows or explicit overrides, with simple snapping and fullscreen as the only layout primitives.

## ADDED Requirements

### Requirement: Windows open maximized by default
Every newly mapped toplevel window SHALL be placed maximized on its target workspace's output, filling the usable area (output minus reserved layer-shell surfaces such as the bar). The system MUST NOT apply any automatic tiling, splitting, or resizing to newly opened windows.

#### Scenario: Regular app window opens
- **WHEN** a window without dialog characteristics is mapped (e.g. a new Ghostty or Zed window)
- **THEN** it fills the usable area of its output and receives keyboard focus

#### Scenario: Second window on the same workspace
- **WHEN** a workspace already has a maximized window and another window is mapped onto it
- **THEN** the new window is maximized on top and becomes the focused window; the previous window remains on the workspace, accessible via focus cycling

### Requirement: Dialog-shaped windows float
A newly mapped window whose xdg-toplevel type is DIALOG, UTILITY, SPLASH, TOOLBAR, or POPUP_MENU, or which sets a transient parent, SHALL be placed floating, centered on its parent (or screen center if no parent), at its requested size, above maximized windows.

#### Scenario: File picker opens
- **WHEN** Chromium opens a file chooser dialog
- **THEN** the dialog floats centered over its parent window at its requested size and takes focus

#### Scenario: Dialog closes
- **WHEN** the floating dialog is dismissed
- **THEN** focus returns to the window that held it before the dialog opened

### Requirement: Per-app placement overrides
The configuration MUST allow per-app placement rules matching on app id (Wayland) or WM_CLASS (XWayland), forcing a window to `maximize` or `float` regardless of its toplevel type, and MUST provide the same control at runtime via IPC for the focused window.

#### Scenario: App spawns normal windows typed as dialogs
- **WHEN** configuration contains a rule matching an app id with `mode = "maximize"` and that app maps a window typed DIALOG
- **THEN** the window is maximized like a regular window

#### Scenario: Float override via IPC
- **WHEN** `lazydeskctl float <window> on` is issued for the focused window
- **THEN** the window switches to floating centered at its chosen size immediately

### Requirement: Half and quarter snapping
The user SHALL be able to snap the focused window to the left, right, top, or bottom half of the usable area, and to any quarter, via keybinding and via IPC. A snapped window MUST be restorable to maximized with a single action. Snapping MUST NOT introduce a persistent layout tree: each window's snap state is independent.

#### Scenario: Snap two windows side by side
- **WHEN** the user snaps one window left and another right on the same workspace
- **THEN** each occupies its half of the usable area without any split container being created

#### Scenario: Restore a snapped window
- **WHEN** a snapped window is restored to maximized
- **THEN** it fills the usable area again and its former snap state is forgotten

### Requirement: Fullscreen toggle
The user SHALL be able to toggle true fullscreen (covering the bar, no reserved areas) for the focused window via keybinding and IPC, and apps requesting fullscreen (e.g. video playback) MUST be honored.

#### Scenario: App requests fullscreen
- **WHEN** a web page enters fullscreen video mode
- **THEN** the Chromium window covers the entire output including the bar area

#### Scenario: User exits fullscreen
- **WHEN** the user triggers the fullscreen toggle while the window is fullscreen
- **THEN** the window returns to its previous maximized (or floating) state

### Requirement: Focus cycling and closing
The user SHALL be able to cycle keyboard focus among all windows on the current workspace (maximized, snapped, and floating) in both directions, and close the focused window via keybinding. Closing MUST first ask the app to close gracefully; a force-kill action MUST be available via IPC.

#### Scenario: Cycle between stacked windows
- **WHEN** two windows exist on a workspace and the user triggers focus-next
- **THEN** the other window is focused and raised if floating

#### Scenario: Close unresponsive window
- **WHEN** `lazydeskctl kill <window>` is issued for a window that ignores a graceful close
- **THEN** the window's client connection is destroyed and it disappears from all listings
