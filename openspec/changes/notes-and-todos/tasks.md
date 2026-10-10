# Tasks

## 1. Storage and verbs

- [ ] 1.1 Implement the markdown stores (note file naming, todo.md parsing with pass-through of non-task lines, tag extraction), verify round-trip: every parse→write cycle preserves unrelated content byte-for-byte
- [ ] 1.2 Implement IPC/CLI verbs `todo add|done|reopen|rm|list` and `note add|list|show` with `--json`, verify the agent-works-the-todo-list scenario
- [ ] 1.3 Implement change detection (mtime short-circuit rescan), verify the agent-edits-the-todo-file scenario (new task visible without restart)

## 2. Capture surfaces

- [ ] 2.1 Implement palette quick capture (`todo <text>` / `note <text>` commit + confirm row; bare `todo` listing with complete/reopen/delete; bare `note` quick-open), verify both capture scenarios incl. focus return
- [ ] 2.2 Add "add todo" / "new note" to the bar action menu via the action registry, verify the capture-from-menu scenario

## 3. Browser and integration

- [ ] 3.1 Implement `lazydesk-notes` overview (full content inline, recency order, day groups, tag filter), verify the zoomed-out-shows-everything and fresh-capture-tops-the-list scenarios
- [ ] 3.2 Implement focused view with editor handoff and filter persistence, verify the zoom-in-filter-edit scenario
- [ ] 3.3 Implement todo lifecycle actions in the browser, verify complete-from-browser rewrites the marker in `todo.md`
- [ ] 3.4 Verify end-to-end parity: the same todo captured/managed via palette, bar menu, cockpit, browser, and CLI stays consistent (single source of truth)
