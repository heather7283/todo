project('todo', 'c',
        version: '0.1.0',
        license: 'GPL-3.0-or-later',
        meson_version: '>= 1.3.0',
        default_options: ['warning_level=3'])

sqlite3_dep = dependency('sqlite3')

getopt_lib = static_library('getopt', ['thirdparty/getopt/getopt.c',
                                       'thirdparty/getopt/getopt_long.c'])

include_dirs = [
  'src',
  'thirdparty/getopt',
]

todo_sources = [
  'src/todo.c',
  'src/utils.c',
  'src/db.c',
  'src/xmalloc.c',
  'src/actions/add.c',
]

executable('todo', todo_sources,
           include_directories: include_dirs,
           link_with: [getopt_lib],
           dependencies: [sqlite3_dep],
           install: true)

