1. Foundation — what are we actually building on?

 1. From scratch or on shoulders? Do you want LazyWM built directly on wlroots (like Sway/hyprland), as a sway fork/IPC companion, or fully from scratch (Raw wayland compositor)? Given you're a C/Rust dev — I'd guess Rust + wlroots-rs or smithay. How
    much of "not invented here" do you tolerate for things like input handling, XWayland, screen capture?

    Everything that is possible to build upon - should be used if they are well established projects. Prefer C if possible, Rust only if you really need it

2. Is the "DE" scope just WM + a few daemons? i.e., are bars/panels/notification daemons/lock screens in scope, or do we compose existing tools (waybar, mako/dunst, swaylock) and LazyWM is really the compositor + session glue?

Yes let's also design bars/panels/notifications so it all works consistenly

 2. The fullscreen-first model — the core design question

 Your description suggests the mental model is "one app = one monitor-full surface", not classic tiling. So:

 3. Tag/workspace semantics: Would you rather have (a) a flat ring of N workspaces you flip through (like dwm tags /bspwm), or (b) one workspace per application role ("pi", "zed", "browser", "chat") where the role is bound at spawn time?

role at spawn time + extra workspaces if needed

 4. Does "fullscreen by default" mean: apps are maximized (still decorated, still resizable) or true fullscreen (no decorations, no way to see anything else)? What happens with a fullscreened Ghostty when you want to compare code with Zed — split?
    overlay? two monitors?

apps maximized - split should be allowed, but not optimized for

 5. Monitors: How many? Do you want a primary development monitor + secondary for Telegram/Chromium, with the WM remembering which workspace lives where?

Design for 1 monitor as I might want to use it on laptop. But allow several monitors and use them

 3. The "accidental small window" problem

 6. When Chromium pops up a file picker or Ghostty spawns a small helper: should it (a) float centered above, (b) tile into a split, (c) be matched by rules you define per-app, or (d) heuristics (dialog-shaped → float, normal → fullscreen)? Do you care
    about keeping it simple: "everything fullscreen unless window type = DIALOG/UTILITY/TOOLTIP → float"?

Yes, later, let's KISS. But allow per app overwride for apps that don't respect type

 7. Pi specifically: you run it inside Ghostty fullscreen. Do you ever need Pi's terminal split vertically for reading its output while typing elsewhere, or is one fullscreen terminal per Pi session fine?

I can split in ghostty. But do make Pi (harness in general) central in design. Let's see for example how Omarchy does it's AI central.

 4. Control surface

 8. Keyboard-first, obviously — which key? Mod4 (Super) free for you, or do you use it for something else (Zed, Ghostty use it?)? Binding style: dwm-like chords, or hyprland-like verbs (super+shift+q etc.)?

Yes, use super. And allow inputting commands to WM/DE with super + enter presumably

 9. How do you switch apps? Muscle memory today: alt-tab? Super+number per workspace? Or "fuzzy jump to app" (like a wm-run prompt)? Since there are ~6 fixed apps, per-app bindings (Super+P → Pi terminal, Super+E → Zed...) might beat a generic alt-tab
    entirely.

Super + number. Fuzzy can be used for openning some other APP. But I like windows style app categorization and search. Use AI to categorize APPs and make a tree based navigation possible

 10. Scratchpad terminals: for "vibe, code, execute bash" — do you want a drop-down terminal (quake-style) summoned over anything for quick one-liners, so you never leave the current context?

 Yes, do a drop down quake like terminal

 5. Session & practicalities

 11. Session startup: should LazyWM auto-spawn your stack on login (pi-ghostty, Zed, Chromium, Telegram on their workspaces), and restore after crash/relogin? Or do you spawn manually?

 Autospawn

 12. Locking/idle: in scope (dev machine, maybe laptop?) or out of scope?

Locking out of scope - just turn off screen and wake up on action

 13. Targets: Void Linux (your machine) first, Wayland-only, XWayland needed for anything? (Telegram Desktop is fine on Wayland; anything legacy?)

XWayland better use for accidental problems. Target - distro agnostic I use both Void/Ubuntu/Debian

 14. Language for the core: you develop in C and Rust — which do you want this project in? (Rust+smithay is the pragmatic modern choice; C+wlroots is closer to dwm philosophy.)

C+wlroots, but modern C23

 Answer whichever ones matter — I'll push back where I think a simpler rule works, and then we'll open the first OpenSpec change proposal with the agreed capability breakdown (compositor core, fullscreen policy, floating exceptions, keymap, session,
 IPC).
