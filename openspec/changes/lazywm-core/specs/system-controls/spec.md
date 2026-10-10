# Spec Delta

## Purpose

Every-day system controls a development daily-driver needs — capture, clipboard history, volume/brightness, battery awareness — built on established utilities rather than custom daemons.

## ADDED Requirements

### Requirement: Screenshot capture
The DE SHALL provide full-screen and interactive-region screenshot capture as rebindable keybindings (defaults `Super+Print` full, `Super+Shift+Print` region) and IPC verbs: captured images save to the configured directory, copy to the clipboard, and trigger a notification with the file path.

#### Scenario: Region screenshot
- **WHEN** the user presses `Super+Shift+Print` and selects a region
- **THEN** the capture saves to the screenshot directory, lands in the clipboard, and a notification names the file

### Requirement: Screen recording
The DE SHALL provide toggleable region screen recording as a keybinding and IPC verb: start begins recording the selected region to a file with a visible recording indicator; stop finalizes the file and notifies with its path.

#### Scenario: Record and stop
- **WHEN** the user starts a region recording, then triggers stop
- **THEN** an indicator was visible during recording and the notification names the finished file

### Requirement: Clipboard history
The DE SHALL maintain a persistent clipboard history (text and images) across applications and reboots, with a viewer overlay (default `Super+v`) offering fuzzy selection; selecting an entry copies it to the clipboard and pastes nothing implicitly. A clear-history action SHALL be available as a verb.

#### Scenario: Recall an earlier copy
- **WHEN** the user presses `Super+v`, fuzzy-selects an entry from an hour ago, and presses `Enter`
- **THEN** that entry is the active clipboard content

### Requirement: Volume and brightness keys with OSD
Volume, mute, and brightness keys SHALL adjust the default sink/source and displays via established audio/display utilities, each keypress showing an on-screen display; media keys SHALL route to the focused player where a player exists.

#### Scenario: Volume key
- **WHEN** the user presses a volume-up key
- **THEN** the default sink volume increases, an OSD appears, and no window loses focus

### Requirement: Battery awareness
On battery-capable machines the DE SHALL notify at a configurable low-battery threshold and with urgency styling at the critical threshold; on machines without batteries this requirement is inert.

#### Scenario: Low battery
- **WHEN** the battery crosses the configured low threshold
- **THEN** a notification appears and the next crossing of the critical threshold raises its urgency

### Requirement: Built on established utilities
System controls SHALL be implemented atop established utilities (e.g. grim/slurp or equivalent capture tools, wf-recorder or equivalent, wl-clipboard + cliphist or equivalent, pactl, brightnessctl, an established OSD) wired through generated keybindings and small scripts. These functions MUST NOT depend on LazyWM's facade daemon being alive.

#### Scenario: Controls survive facade restart
- **WHEN** `lazywmd` is stopped and the user takes a screenshot, adjusts volume, and opens clipboard history
- **THEN** all three still function
