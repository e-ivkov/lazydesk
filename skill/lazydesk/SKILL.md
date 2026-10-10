---
name: lazydesk
description: Control and reconfigure the LazyDesk desktop (workspaces, windows, spawns, config) via lazydeskctl IPC and TOML config.
---

# LazyDesk agent skill

LazyDesk is a development-first desktop profile on Hyprland. As an agent you
control it **only** through public tooling — no privilege path exists:

## Control: lazydeskctl

    lazydeskctl --json windows list        # all windows: id, workspace, archetype, app_id, state
    lazydeskctl --json workspaces list     # workspaces: number, name, window count, urgency
    lazydeskctl --json outputs list
    lazydeskctl workspace focus <n|name>   # e.g. focus 1 (agent) — same as Super+1
    lazydeskctl window focus|close|kill <id>
    lazydeskctl window move <id> <n|name>
    lazydeskctl window snap <id> left|right|top|bottom|restore
    lazydeskctl window float <id> on|off
    lazydeskctl spawn --archetype <name> -- <cmd...>   # window lands on that archetype's workspace
    lazydeskctl config reload               # regenerates artifacts + reloads backend, emits config.reloaded
    lazydeskctl watch                       # live event stream (NDJSON)

Archetypes (shipped defaults, 1–7 + unnumbered):
agent, editor, terminal, browser, messenger, cockpit, lazygit, desk, files.

## Configure: TOML under ~/.config/lazydesk/

`lazydesk.toml` is the single source of truth (archetype bindings, bindings,
window rules, spawn entries, idle, theme). Generated artifacts in backend
locations carry `# lazydesk-managed` markers — never hand-edit those; edit the
TOML and run `lazydeskctl config reload`. Invalid config is rejected with the
offending key named; previous-good config stays active.

## Safe reconfiguration

1. Read: `cat ~/.config/lazydesk/lazydesk.toml`
2. Edit the TOML (keep unknown keys; comments allowed)
3. Validate+apply: `lazydeskctl config reload`
4. Check the result event; on error, fix the key named in the message

## Related commands

    lazydesk doctor            # session health, archetype completeness
    lazydeskctl apps categorize  # regenerate the palette app tree (calls you non-interactively)

(Full IPC reference: docs/ipc.md — every action available to keybindings and
the bar menu is available here.)
