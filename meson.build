project('todo', 'c',
        version: '0.1.0',
        license: 'GPL-3.0-or-later',
        meson_version: '>= 1.3.0',
        default_options: ['warning_level=3'])

sqlite3_dep = dependency('sqlite3')

ccronexpr_lib = static_library('ccronexpr',
                               'thirdparty/supertinycron/ccronexpr.c',
                               c_args: ['-DCRON_USE_LOCAL_TIME'])
getopt_lib = static_library('getopt',
                            ['thirdparty/getopt/getopt.c', 'thirdparty/getopt/getopt_long.c'])

include_dirs = [
  'src',
  'thirdparty/supertinycron',
  'thirdparty/getopt',
]

todo_sources = [
  'src/todo.c',
  'src/utils.c',
  'src/db.c',
  'src/xmalloc.c',
  'src/actions/add.c',
  'src/actions/check.c',
  'src/actions/dismiss.c',
]

executable('todo', todo_sources,
           include_directories: include_dirs,
           link_with: [ccronexpr_lib, getopt_lib],
           dependencies: [sqlite3_dep],
           install: true)

