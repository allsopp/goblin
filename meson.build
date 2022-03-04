project('goblin', ['c'],
  version: '0.1-alpha',
  meson_version: '>=0.53.2',
  default_options: ['warning_level=3',
                    'c_std=c99'])
assert(host_machine.endian() == 'little')

deps = []
libs = []
incs = ['include']

cc = meson.get_compiler('c')
deps += cc.find_library('libz')

goblin = shared_library('goblin', ['goblin.c'],
                        include_directories: incs,
                        dependencies: deps)
dep = declare_dependency(include_directories: incs,
                         link_with: goblin)

sizes = [1, 2, 3, 4, 5, 6, 7, 8, 16, 32, 64, 128, 256]
png2tga = executable('png2tga', 'png2tga.c',
                     include_directories: incs,
                     link_with: goblin)
test_goblin = find_program('test_goblin.sh')
foreach s : sizes
  test('@0@ x @0@'.format(s), test_goblin, args: [png2tga.full_path(), s.to_string()])
endforeach
