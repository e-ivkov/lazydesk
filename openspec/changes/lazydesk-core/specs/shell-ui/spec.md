# Spec Delta

## Purpose

Defines the minimal consistent shell around the WM: the top bar and notification behavior, both driven by the same palette and IPC so the DE looks and behaves as one product.

## ADDED Requirements

### Requirement: Minimal top bar
A layer-shell top bar SHALL render on every output showing: archetype and ad-hoc workspace indicators with focus and urgency state, the focused window title, and a clock. Bar visibility and height are configurable per output.

#### Scenario: Workspace state visible
- **WHEN** Telegram requests attention on `messenger` while `agent` is focused
- **THEN** the `messenger` indicator shows an urgency marker without the user switching workspaces

### Requirement: Bar interaction and geometry
The bar MUST subscribe to IPC events and update without polling; clicking a workspace indicator focuses that workspace. Bar geometry is reserved via layer-shell exclusive zone so maximized windows never render under it; hiding the bar MUST return its exclusive zone to windows.

#### Scenario: Click to focus
- **WHEN** the user clicks the `editor` workspace indicator
- **THEN** the zed workspace is focused, same as `Super+2`

### Requirement: Harness status indicator
The bar SHALL show a default-harness indicator that is active when the harness process is running, with the archetype marker of the harness. Activating it (click or configured key) is equivalent to `Super+a` focus-or-spawn.

#### Scenario: Harness indicator click
- **WHEN** the user clicks the harness indicator while Zed is focused
- **THEN** the pi workspace is focused with the harness terminal on top

### Requirement: Bar action menu with shortcut parity
The bar SHALL provide a dropdown menu listing every user action (bound or bindable), organized by the binding groups. Each menu item MUST show its current keybinding alongside the action name where one is bound, and activating an item MUST perform exactly the same operation as the binding (same action registry, same IPC verbs). The menu MUST be reachable by pointer click on every output's bar and include a "show keybindings" entry.

#### Scenario: Menu item equals keybinding
- **WHEN** the user clicks "new workspace" in the bar menu
- **THEN** the effect is identical to pressing `Super+n`

#### Scenario: Rebound key reflected in menu
- **WHEN** a binding is rebound via settings and config is reloaded
- **THEN** the bar menu shows the new key combination next to the action without restart

### Requirement: Keybinding cheatsheet
The "show keybindings" bar menu entry SHALL open an overlay listing all current bindings grouped by function, generated from the live binding registry, dismissible with `Escape`, and available agentless.

#### Scenario: Cheatsheet reflects rebinds
- **WHEN** the user rebinds an action and reopens the cheatsheet
- **THEN** the new combination is shown for that action

### Requirement: Consistent notifications
Notifications SHALL be rendered by a notification daemon (mako) themed from the shared LazyDesk palette and configured by the session. Notifications MUST appear above maximized windows and the bar, respect urgency styling, and MUST NOT steal keyboard focus. A DND toggle SHALL be exposed via palette verb and IPC.

#### Scenario: Spawn failure notifies
- **WHEN** an autospawn entry fails
- **THEN** a notification styled with the session palette appears and no window loses focus

#### Scenario: DND via palette
- **WHEN** the user runs the DND verb in the palette
- **THEN** subsequent notifications are suppressed until toggled off, and the state is reflected in the bar

### Requirement: One shared theme with shipped presets
The bar, palette, and notification styling SHALL derive from a single minimal theme definition in the shared configuration (color tokens, font, corner radius — no further styling surface), such that changing the theme and reloading restyles all shell components consistently. Switching themes MUST be possible via settings, palette verb, and IPC.

#### Scenario: Theme change restyles shell
- **WHEN** the user switches the theme in configuration and reloads
- **THEN** bar, palette, and notifications all adopt the new palette on next render

### Requirement: Shipped theme presets
The DE SHALL ship a small set of common themes as token files, selected by name in configuration. The set MUST include `catppuccin-mocha` (the default), `catppuccin-latte` (light), and at least two other widely used dark themes.

#### Scenario: Light theme switch
- **WHEN** the user switches from `catppuccin-mocha` to `catppuccin-latte` via the palette
- **THEN** all shell surfaces render with the light palette and text remains readable everywhere
