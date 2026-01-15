project('pscm', 'c',
  default_options: ['c_std=c99']
)

add_global_arguments('-D_GNU_SOURCE', language: 'c')

inc = include_directories('.')

lib_sources = [
  'value.c',
  'vm.c',
  'reader.c',
  'builtin.c',
  'json.c',
  'api.c'
]

lib = static_library('pscm', lib_sources, include_directories: inc)

executable('pscm', 'main.c', link_with: lib, include_directories: inc)