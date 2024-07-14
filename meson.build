
project('duckydd', 'c', version: '0.3.4')
compiler = meson.get_compiler('c')

sources = []
incdir = []

incdir += include_directories('include')

subdir('lib')
subdir('src')

dep = []
dep += dependency('libudev')
dep += compiler.find_library('m', required: false)

if dependency('xkbcommon-x11').found()
  dep += dependency('xkbcommon')
  dep += dependency('xkbcommon-x11')
  dep += dependency('xcb')
endif

add_project_arguments('-DENABLE_XKB_EXTENSION', language: 'c')

executable('duckydd', sources,
  include_directories: incdir,
  dependencies: dep)
