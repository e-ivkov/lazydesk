# Tasks

## 1. Schema and model

- [ ] 1.1 Author `docs/config-schema.toml` (keys, types, defaults, widget hints, archetype alternatives) from the foundation config surface, verify every documented LazyWM key has an entry
- [ ] 1.2 Implement schema-driven config model (load, validate, serialize with unknown-key and comment preservation), verify round-trip byte-stability on fixture files via property test

## 2. Application shell

- [ ] 2.1 Create `lazywm-settings` GTK4 app on the `lazywm` archetype with generated theme CSS from `theme.toml`, verify summon scenario (`Super+comma`)
- [ ] 2.2 Implement section navigation (archetypes, spawn, bindings, rules, scratchpad, outputs/idle, categories, theme, advanced/unknown), verify each section renders its schema keys

## 3. Editors and validation

- [ ] 3.1 Implement archetype binding editors with alternative selection, verify swapping `editor` in the UI equals editing the TOML by hand (edit-equivalence scenario)
- [ ] 3.2 Implement keybinding editor with press-to-record and conflict detection, verify recorded-binding and conflicting-binding scenarios
- [ ] 3.3 Implement remaining editors (rules, spawn entries, scratchpad, idle, categories tree, theme picker) with inline validation, verify an invalid value refuses write with reason
- [ ] 3.4 Implement apply pipeline: atomic write → IPC reload → event ack, verify reload-error surfacing inline

## 4. Concurrency and completeness

- [ ] 4.1 Implement mtime/hash conflict detection with external/local/diff resolution, verify the agent-edits-while-open scenario
- [ ] 4.2 Implement advanced/unknown key listing with preservation, verify unknown-key scenario
- [ ] 4.3 CI check: every schema key maps to a widget and every documented setting is reachable, verify test fails when a key is deliberately orphaned
