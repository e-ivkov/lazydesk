wm_args = ['-DWLR_USE_UNSTABLE']
wm_deps = [
  dependency('wlroots-0.20'),
  dependency('wayland-server'),
  dependency('xkbcommon'),
]
executable('lazydesk-wm', ['main.c', 'windows.c', 'workspace.c', 'input.c', 'ipc.c'],
  include_directories : [root_inc, include_directories('..')],
  c_args : wm_args,
  dependencies : wm_deps + [lazydesk_common_dep],
  install : true)
