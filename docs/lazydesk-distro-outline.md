# LazyDesk Distro — outline (parked for later)

Status: idea note from a design conversation (2026-10). Not committed to. LazyDesk the
DE stays distro-agnostic; this is the possible endgame layer *under* it.

## Framing

"Distro without a package manager" really means: whose package manager. Any from-source
system converges on manifests + build graph + caching (= a PM you wrote). The real design
choice is the **unit of composition** and **where mutation happens**.

## The strong version: system-as-artifact (immutable A/B images built from source)

- One git repo *is* the distro: kernel pin (LTS), glibc, coreutils, lib stack,
  toolchains, LazyDesk — each a source manifest (hashes, build graph).
- A build driver (~2k LOC, ours; the repo *is* the dependency closure, no resolution
  drama) produces a bootable rootfs image. Update = build image N+1, reboot into it.
  Rollback = bootloader picks image N. The running system has no PM because /usr is
  immutable; mutation happens at build time, in git, diffable and revertable.
- Layout: read-only /usr, /etc as git overlay, persistent /var.
- Custom gccs never in /usr: first-class build targets installed to /opt/tc/gcc-15/,
  switched per shell per project (postgres/greenplum multi-compiler workflows).
- CVE flow: bump manifest, rebuild the affected slice (ccache), release image.
- Bootstrap: seed the first image from a Void or Debian stage, converge release over
  release until the base is fully ours. Nobody bootstraps gcc from scratch twice.
- Init: dinit or runit (no systemd assumption, matches LazyDesk session model).
- Completes the metaphor: desktop repo = TOML -> generated artifacts;
  system repo = manifests -> generated image. Same philosophy all the way down.

## Why it fits this user

1. Multi-gcc as first-class concern (greenplum pinned compilers, postgres mainline
   tested against many).
2. Immutable /usr enforces project-toolchain hygiene.
3. Source staleness stops being a distro decision (the Void ghostty/hyprland class of
   problem disappears).
4. Rollback is a bootloader entry, not a prayer.
5. A/B images make scary upgrades (glibc) testable against the old image first.

## Costs (accepted eyes-open)

- Security response is ours (openssl/openssh/curl CVEs while busy = the real price).
- glibc upgrades are the scary part; upgrade rarely, test first via A/B.
- Bootstrap is weekends; steady state 1-2 h/week + incidents.
- Hardware quirks land on us.

## Options ranked

1. Image-based from-source (above) — the right ambitious answer.
2. Overlay distro — keep Void/Debian base, own the ~50 packages that matter
   (ghostty, niri, gccs, postgres tooling) into /usr/local + /opt/tc/*. 80/20.
3. Gentoo-style own ports tree — rejected: reinventing portage, poorly.
4. Nix shell on a boring host — violates the no-PM constraint; noted as the cheap
   version of the same toolchain win if host purity is ever relaxed.

## Sequencing

Do not couple the projects. Finish the DE (distro-agnostic by design: TOML ->
emitters -> facade; niri/quickshell/ghostty build fine from manifests). Prototype the
distro in a VM/second disk as option 2 seeded from Void; promote to primary if the
friction log shows base-distro pain rather than desktop pain.

## Open questions for later

- glibc vs musl (glibc: greenplum/orca pragmatics).
- Init system (dinit vs runit).
- Boot/loader for A/B (limine vs systemd-boot).
- Binary cache strategy for CI-speed rebuilds (ccache artifacts, remote cache).
- Where VMs/containers fit for exotic build environments.
