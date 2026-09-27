# nnware

This repository includes:
- Decompilation of NintendoWare modules that are linked statically in some Nintendo games.

Do not ask for or PR any copyrighted material to this repo. You will be ignored.
Usage of the materials here outside of matching decompilation is not supported.

# Credits
 - [Shadow](https://github.com/shadowninja108/) - For [Skyline](https://github.com/shadowninja108/Skyline), which was the primary inspiration and use case for this repo.
 - [Shibbo](https://github.com/shibbo) - For [OdysseyReversed](https://github.com/shibbo/OdysseyReversed/), which most of these headers came from.

# Development
For running commands please install [task](https://taskfile.dev), CMake, Ninja and Clang/LLVM.

The Clang/LLVM toolchain is only used for formatting and running `clang-tidy`.
The version should match the [image used in the PR](https://github.com/open-ead/containers/blob/main/ubuntu-builder/Dockerfile)

Before you begin, run `task clone-deps` to clone dependencies.

Run `task pr` to run the same checks that the PR runs. Run `task fix` to fix the clang-format issues.


