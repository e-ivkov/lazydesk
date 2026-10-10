project('lazydesk', 'c',
  version : '0.1.0',
  license : 'MIT',
  default_options : ['c_std=c23', 'warning_level=2', 'buildtype=debugoptimized'])

cc = meson.get_compiler('c')

prefix    = get_option('prefix')
bindir    = join_paths(prefix, get_option('bindir'))
pkgdatadir = join_paths(prefix, get_option('datadir'), 'lazydesk')

cfg = configuration_data()
cfg.set_quoted('LAZYDESK_VERSION', meson.project_version())
cfg.set_quoted('LAZYDESK_PKGDATADIR', pkgdatadir)
cfg.set_quoted('LAZYDESK_BINDIR', bindir)
configure_file(output : 'lazydesk-build.h', configuration : cfg)

root_inc = include_directories('.')

add_project_arguments('-DHAVE_CONFIG_H', language : 'c')

subdir('vendor/tomlc')
subdir('vendor/cjson')
subdir('lib')
subdir('daemon')
subdir('ctl')
subdir('menu')
subdir('tools')
subdir('wm')
subdir('tests')

# Data, themes, config templates, skill, session script
install_subdir('themes',  install_dir : pkgdatadir, exclude_directories : [])
install_subdir('config',  install_dir : pkgdatadir)
install_subdir('data/deps', install_dir : join_paths(pkgdatadir, 'deps'), strip_directory : true)
install_subdir('skill',   install_dir : pkgdatadir)
install_data('session/lazydesk-session', install_dir : get_option('bindir'))
install_data('data/wayland-sessions/lazydesk.desktop',
  install_dir : join_paths(prefix, get_option('datadir'), 'wayland-sessions'))
