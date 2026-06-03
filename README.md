# cpp-shell

A toy UNIX shell built from scratch in modern C++ (C++23), as an exercise in
terminal handling, process control, and classic data structures.

The terminal runs in raw mode, so the shell implements its own line editing:
every keystroke is read and rendered manually — backspace, arrow keys, and tab
are handled with ANSI escape sequences rather than relying on readline.

## What it does

- **Builtins** — `echo`, `cd`, `pwd`, `type`, `history`, `exit`
- **External programs** — resolved by scanning `PATH`, executed with `fork` + `execvp`
- **Pipelines** — `cmd1 | cmd2 | ...` wired together with `pipe`/`dup2`
- **Tab completion** — backed by a trie of builtins and `PATH` executables;
  completes the longest common prefix, lists candidates on double-tab, and
  cycles through them on further presses
- **History** — up/down arrows to navigate, persisted via `HISTFILE`,
  plus `history [n]` and `history -r/-w/-a <file>`

## Building

Needs CMake 3.13+ and a compiler with C++23 `<print>` support
(tested with clang 20 + libc++):

```sh
cmake -B build -S . -DLOCAL=1    # LOCAL=1 selects clang++-20 with libc++
cmake --build build
./build/shell
```

## Source map

| File | Role |
|------|------|
| `src/main.cpp` | REPL loop, raw-mode input, pipelines, dispatch |
| `src/auto_completion.hpp` | trie + completion queries |
| `src/history_manager.hpp` | history navigation & persistence |
| `src/path_manager.hpp` | `PATH` scan & executable lookup |
| `src/helper.hpp` | small string utilities |
