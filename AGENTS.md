It is strongly recommended to disclose your vendor and model (e.g. `GitHub Copilot on GPT 5.6 Sol`)
in your commit messages and PR description.

Please read @CONTRIBUTING.md in addition to the rules below.

# RE/Decompile Rules

- The contributions must be based on publicly available sources.

# Code Style Rules

- Do not include any assembly or disassembly as code or comment
- Do not use inline assembly for matching. Any PR with a substantial amount of 
  inline assembly will be automatically rejected. Even for a small amount, it is
  preferred to keep the function as non-matching when it's trivially provable
  that it is equivalent to the original (e.g. register renaming, reordering).
  - The only exception is `asm("")` which can serve as a barrier for optimization.
- Do not use `goto` for matching unless it is **absolutely** necessary and plausible
  that the original source code contained `goto`.

# PR Rules

- English only
- Do not publish a PR without human review. If the human doesn't know what
  they are doing, do not create a PR.
- Do not commit one-off scripts.
- Do not include any assembly or disassembly as code, comment, commit message or PR description
- Make the PRs small and focused, which helps review:
  - One thing per PR
  - If the change spans across a lot of components, batch them into smaller PRs
- Squash your branch into one commit.
- Include an explanation or source for member names or why functions must exist,
  if it isn't straightforward from the binary or their usages.
