# Tasks

## 1. Client skeleton and surface

- [ ] 1.1 Create `lazywm-cockpit` process: window on the `cockpit` archetype, theme-token rendering, section layout skeleton, verify `Super+c` summon per the surface scenario
- [ ] 1.2 Implement IPC subscription (workspace/window/focus events) and snapshot store with rwlock, verify the dashboard updates when windows open/close on another workspace

## 2. Collectors

- [ ] 2.1 Implement `/proc` collectors (CPU, memory, disk usage+IO, network, temps/battery) with interval refresh, verify live-update scenario
- [ ] 2.2 Implement devices listing (outputs, inputs, storage) reacting to hotplug events, verify the hotplugged-output scenario
- [ ] 2.3 Implement WM section: workspaces/window counts, focus-time accumulation, IPC counters, config health from the extended `config.reloaded` event, verify the config-error scenario
- [ ] 2.4 Implement AI section with agentless badge, verify the agentless badge scenario

## 3. Projects and work analysis

- [ ] 3.1 Implement libgit2-based project facts (branch, dirty, last commit) with async scan and budgets, verify a 10k-file repo scan leaves the UI responsive
- [ ] 3.2 Implement language/size histogram with ignore patterns, verify `node_modules` and build dirs are skipped and counts match a known fixture repo
- [ ] 3.3 Implement ranking (decayed git activity + persisted focus time) and pinned projects, verify pinned-stays-top and top-used ordering scenarios
- [ ] 3.4 Implement `~/work` top-files and type-breakdown report, verify the recently-modified scenario
- [ ] 3.5 Implement todos and recent-notes panels over the shared notes stores with complete/reopen and open-in-editor actions, verify both cockpit scenarios in the cockpit spec (depends on the notes-and-todos change's verbs)

## 4. AI summaries and actions

- [ ] 4.1 Implement the shared harness-invocation helper with TTL cache under `~/.cache/lazywm/ai/`, verify agentless instant-degradation and cached-result reuse before TTL
- [ ] 4.2 Wire project/work AI summaries with commit-based invalidation, verify agentless project-entry scenario
- [ ] 4.3 Implement entry actions (open in editor / file manager / copy path) via public IPC only, verify the open-in-editor scenario and the no-side-effects scenario (`git status` unchanged, no mtimes outside cache)

## 5. Integration

- [ ] 5.1 Verify the action-reproducibility scenario: replay cockpit's IPC ops from a script and get identical results
- [ ] 5.2 Document cockpit configuration (pins, TTLs, budgets, intervals) in `docs/cockpit.md` and verify every documented key takes effect
