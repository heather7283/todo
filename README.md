# todo - a CLI TODO app
todo is a simple CLI app to keep track of tasks. It is inspired by [tore].

## Installation
```sh
git clone 'https://github.com/heather7283/todo.git'
cd todo
meson setup build
meson compile -C build
sudo meson install -C build
```

## Usage
```sh
# todo add deadline UNIX_SECONDS TITLE [BODY]
# I will add ability to specify date in other formats later, use date hack for now
todo add deadline "$(date -d 'next Thursday' '+%s')" 'Buy milk' 'Optional detailed description...'
# todo add periodic CRON_EXPR TITLE [BODY]
# see crontab(5)
todo add periodic '0 8,20 * * *' 'Feed cat'
todo add periodic '0 0 31 12 *' 'Celebrate new year'
# todo add idle TITLE [BODY]
# "idle" tasks do not have a deadline, nor are they periodic
todo add idle 'Find a gf'
```
Intended usage is to call `todo check` in your shell startup file
to get reminded of all todo items every time new shell is launched.

## References:
- https://github.com/rexim/tore
- https://github.com/exander77/supertinycron
- https://github.com/staticlibs/ccronexpr

[tore]: https://github.com/rexim/tore

