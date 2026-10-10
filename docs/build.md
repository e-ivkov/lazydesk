# Building LazyDesk

Toolchain: C23 (gcc >= 14 or clang >= 18), meson >= 1.5, ninja.

    meson setup build
    ninja -C build
    meson test -C build

## Per-distro dependencies (backend + components)

### Void Linux (xbps)

    sudo xbps-install -S Hyprland waybar mako ghostty chromium Telegram-desktop \
      thunar lazygit grim slurp wl-clipboard cliphist brightnessctl pipewire seatd

Note: Void's ghostty may lag upstream; a side-by-side AppImage works —
`lazydesk doctor` only requires a `ghostty` in PATH.

### Debian (apt)

    sudo apt install hyprland waybar mako-notifications ghostty chromium \
      telegram-desktop thunar lazygit grim slurp wl-clipboard cliphist \
      brightnessctl pipewire

### Ubuntu (apt)

Same as Debian (universe repo enabled; hyprland from 25.04+).

## Runtime pieces LazyDesk provides itself

lazydesk (install/update/doctor), lazydeskd (IPC facade), lazydeskctl,
lazydesk-menu (palette), session script, themes, agent skill.
