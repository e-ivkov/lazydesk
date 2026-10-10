lazydesk_common = static_library('lazydesk-common',
  ['distro.c', 'deps.c', 'config.c', 'bindings.c', 'generate.c'],
  include_directories : [include_directories('..'), root_inc],
  dependencies : [tomlc_dep, cjson_dep])

lazydesk_common_dep = declare_dependency(
  link_with : lazydesk_common,
  include_directories : [include_directories('..'), include_directories('.')],
  dependencies : [tomlc_dep, cjson_dep])
