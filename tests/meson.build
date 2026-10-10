test_json = executable('test-json', 'test_json.c', dependencies : [cjson_dep])
test_toml = executable('test-toml', 'test_toml.c', dependencies : [tomlc_dep])

test('json round-trip', test_json,
  args : [join_paths(meson.source_root(), 'tests/fixtures/ipc-examples.json')])
test('toml parse', test_toml,
  args : [join_paths(meson.source_root(), 'config/lazydesk.toml'),
          join_paths(meson.source_root(), 'themes/catppuccin-mocha.toml')])
test('cli acceptance', find_program('cli.sh'),
  args : [join_paths(meson.build_root(), 'tools/lazydesk'), meson.source_root()])
test('regen acceptance', find_program('regen.sh'),
  args : [join_paths(meson.build_root(), 'tools/lazydesk'), meson.source_root()])
test('ipc acceptance', find_program('ipc.sh'),
  args : [join_paths(meson.build_root(), 'ctl/lazydeskctl'),
          join_paths(meson.build_root(), 'daemon/lazydeskd'),
          meson.source_root()])

test('wm acceptance', find_program('wm.sh'),
  args : [join_paths(meson.build_root(), 'wm/lazydesk-wm'),
          join_paths(meson.build_root(), 'ctl/lazydeskctl')])