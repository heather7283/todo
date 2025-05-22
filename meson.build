project('todo', 'c',
        version: '0.1.0',
        license: 'GPL-3.0-or-later',
        meson_version: '>= 1.3.0',
        default_options: ['warning_level=3'])

sqlite3_dep = dependency('sqlite3')

todo_sources = [
  'src/todo.c'
]

executable('todo',
           todo_sources,
           dependencies: [sqlite3_dep],
           install: true)

