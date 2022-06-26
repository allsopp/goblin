project('goblin', ['c'],
  version: '0.1-alpha',
  meson_version: '>=0.53.2',
  default_options: ['warning_level=3',
                    'c_std=c99'])
assert(host_machine.endian() == 'little')

deps = []
libs = []
incs = ['include']

incs += get_option('prefix') / 'include'
libs += get_option('prefix') / 'lib'
cc = meson.get_compiler('c')

if host_machine.system() == 'windows'
  zlib = 'zlib'
else
  zlib = 'libz'
endif

deps += cc.find_library(zlib, dirs: libs)

goblin = static_library('goblin', ['goblin.c'],
                 include_directories: incs,
                 dependencies: deps)
dep = declare_dependency(include_directories: incs,
                         link_with: goblin)

png2tga = executable('png2tga', 'png2tga.c',
                     include_directories: incs,
                     link_with: goblin)

test_square = find_program('test/square.py')
sizes = [1, 2, 3, 4, 5, 6, 7, 8, 16, 32, 64, 128, 256]
foreach s: sizes
  test('@0@ x @0@'.format(s), test_square, args: [png2tga.full_path(), s.to_string()], is_parallel: false)
endforeach
