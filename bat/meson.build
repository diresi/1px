project('1pxbat', 'cpp',
  default_options : ['cpp_std=c++20'])
dep_x11 = dependency('X11')
dep_xfixes = dependency('xfixes')
dep_cairo = dependency('cairo')
executable('1pxbat', 'bat.cpp', dependencies : [dep_x11, dep_xfixes, dep_cairo])
