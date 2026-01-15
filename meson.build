project('pscm', 'c',
  default_options: ['c_std=c99']
)

add_global_arguments('-D_GNU_SOURCE', language: 'c')

inc = include_directories('src')

lib_sources = files(
  'src/value.c',
  'src/vm.c',
  'src/reader.c',
  'src/builtin.c',
  'src/json.c',
  'src/api.c'
)

lib = static_library('pscm', lib_sources, include_directories: inc)

executable('pscm', 'main.c', link_with: lib, include_directories: inc)