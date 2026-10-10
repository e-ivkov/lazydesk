wm_args = ['-DWLR_USE_UNSTABLE']
wm_deps = [
  dependency('wlroots-0.20'),
  dependency('wayland-server'),
  dependency('xkbcommon'),
]
executable('lazydesk-wm', 'main.c',
  include_directories : root_inc,
  c_args : wm_args,
  dependencies : wm_deps,
  install : true)
