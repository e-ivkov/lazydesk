# Proposal

## Why

The developer's attention is split across system health, WM state, AI activity, and a growing pile of projects. A single always-one-keystroke dashboard — owned by the DE itself, consistent with its theme, driven by the same IPC and harness layer — replaces ad-hoc htop/git/df habits and gives the agent a place to *read* what matters before acting. General-purpose DEs have nothing like it; this is a development-first differentiator.

## What Changes

Introduces `lazywm-cockpit`, a custom dashboard surface living on the `cockpit` archetype workspace (foundation change defines the archetype; default summon `Super+c`):

- Live system and devices overview: CPU/memory/swap, disk usage and I/O, network I/O, temperature and battery where available; outputs, input devices, storage.
- WM overview: workspaces with window counts, focus-time statistics, config health (last reload result), IPC operation counters.
- AI overview: default harness configured/running state, last app-categorization result, cached AI summary freshness, agentless badge in fallback mode.
- Projects panel: `~/projects` scan producing pinned + top-used projects (ranked by git activity recency and tracked focus time), each with branch, dirty state, last commit, per-language breakdown, and a cached AI summary of recent edits (git-stat only when agentless); actions to open in editor, open in file manager, copy path.
- Work panel: `~/work` top files by recency and size with a type/language breakdown report and optional AI summary.
- Read-only analysis guarantee: cockpit never modifies projects or work data; all actions go through public IPC.

## Capabilities

### New Capabilities

- `cockpit` — the dashboard surface: data sections, refresh model, ranking, actions, agentless degradation, read-only boundaries.

### Modified Capabilities

(none — foundation's `workspaces` spec already defines the cockpit archetype slot; this change fills it)

## Impact

- New binary `lazywm-cockpit` in the monorepo (cairo/pango client, no new runtime deps beyond foundation's), new IPC query ops if focus statistics are not already exposed, new `lazywmctl ai` subcommand surface for cached summaries shared with `apps categorize`.
- Depends on change `lazywm-core` (IPC, archetypes, theme, harness invocation helper).
- Reads `~/projects` and `~/work`; writes only to `~/.cache/lazywm/`.
