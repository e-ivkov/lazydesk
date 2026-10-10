# LazyDesk IPC

Protocol: newline-delimited JSON over `$XDG_RUNTIME_DIR/lazydesk/ipc.sock`.
(Full reference lands with task group 4 — this file will contain every op and
event with verbatim examples, kept in sync with `lazydeskctl`.)

Planned op vocabulary (from the lazydesk-core spec):

    {"op": "version"}                       -> {"protocol": 1, ...}
    {"op": "windows.list"}                  -> [{"id":..., "workspace":..., "archetype":..., "app_id":..., "state":...}]
    {"op": "workspace.focus", "name": "agent"}
    events: window.map/unmap/focus/title/urgency, workspace.focus/urgency,
            config.reloaded, session.spawn-result
