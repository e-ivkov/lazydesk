lazydesk_common = static_library('lazydesk-common',
  ['distro.c', 'deps.c'],
  include_directories : include_directories('..'),
  dependencies : [tomlc_dep])

lazydesk_common_dep = declare_dependency(
  link_with : lazydesk_common,
  include_directories : [include_directories('..'), include_directories('.')],
  dependencies : [tomlc_dep])
