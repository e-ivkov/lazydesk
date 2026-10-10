# Building LazyDesk

Toolchain: C23 (gcc >= 14 or clang >= 18), meson >= 1.5, ninja.

    meson setup build
    ninja -C build
    meson test -C build

## Per-distro dependencies (backend + components)

### Void Linux (xbps)

    sudo xbps-install -S niri quickshell ghostty chromium Telegram-desktop \
      thunar lazygit grim slurp wl-clipboard cliphist brightnessctl pipewire seatd

Note: Void's ghostty may lag upstream; a side-by-side AppImage works —
`lazydesk doctor` only requires a `ghostty` in PATH.

### Debian (apt)

    sudo apt install niri quickshell ghostty chromium \
      telegram-desktop thunar lazygit grim slurp wl-clipboard cliphist \
      brightnessctl pipewire

### Ubuntu (apt)

Same as Debian (universe repo enabled; hyprland from 25.04+).

## Login managers

- **emptty** (Void default): scans only `/usr/share/wayland-sessions/` (or
  `WAYLAND_SESSIONS_PATH`) plus `~/.local/share/wayland-sessions/`. LazyDesk's
  entry installs to `<prefix>/share/wayland-sessions/`; with emptty, also copy
  it: `install -Dm644 /usr/local/share/wayland-sessions/lazydesk.desktop
  ~/.local/share/wayland-sessions/lazydesk.desktop` (the install CLI does this
  when it detects emptty).
- GDM/greetd/other XDG-compliant DMs pick the entry up from `$XDG_DATA_DIRS`.

## Runtime pieces LazyDesk provides itself

lazydesk (install/update/doctor), lazydeskd (IPC facade), lazydeskctl,
lazydesk-menu (palette), session script, themes, agent skill.
