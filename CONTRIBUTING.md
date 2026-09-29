# Contribution Guidelines

## Development

To build and run checks, please install:
- [task](https://taskfile.dev) for running commands.
- CMake (check the version requirement in CMakeLists.txt) and Ninja.

Please run `task clone-deps configure` before running the other
commands.

Workflows:
- Run `task configure` to configure the CMake projects.
- Run `task build` to build and run tests.
- Run `task check` to check formatting and clang-tidy issues. `task fix` formats the files.
- Run `task pr` to run the same checks that the PR runs.
  It's a good idea to always run this one before you open or update a PR.

If there are errors after merging upstream updates, run `task clean clone-deps`
to clean CMake cache and update your local dependencies. If the error
persists, please ask for help in Discord.

## Repo Structure

- `/lib/*`: One CMake subdirectory for each library target that gets statically linked into the game.
- `/test`: Test projects.

## Module Structure

Each SDK module is its own CMake subdirectory at `/lib/` and has its own `include` and `src`
directories. For example for the module `atk`:

```
lib/atk/
├── include/nn/
│   ├── atk/
│   └── atk.h
├── src/atk/
│   └── atk_Something.cpp
└── CMakeLists.txt
```

The public header structure in `lib/*/include/nn` should follow the [same rules as nnSdk headers](https://github.com/open-ead/nnsdk/blob/main/CONTRIBUTING.md).

If there needs to be a new module, you can copy one of the existing modules
and rename the CMake target in `lib/*/CMakeLists.txt`. Then, add the new module
in the following places:
- `/CMakeLists.txt`: Add a `add_subdirectory` near the end of the file.
- `/Taskfile.yml`: Add it to the `MODULES` list near the top of the file.
- `/test/header_check/CMakeLists.txt`: Add it to the `HEADER_CHECK_TARGETS` list.

## PR Rules

Please use English for PRs and squash your branch into one commit.
Keep PRs reasonably sized to get them reviewed faster. If there are multiple
things in the same PR, consider splitting them into multiple PRs.

If it isn't straightforward from the usage or binary why something must be added,
please include an explanation or source.
