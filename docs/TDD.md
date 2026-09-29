# Implementation evidence

Sprint base: `94d14d0eee88f25b89cbe71eb020611042573804`.
The original game was inspected during planning; tests for the new core precede its implementation. This record is evidence, not a second task checklist. Actual commits are identified by task ID in Git history.

## T001 — isolated reset

Red: `make test`, exit 2, after the compiler/test scaffold was working:

```text
Undefined symbols for architecture arm64:
  "_space_default_config", referenced from:
  "_space_init", referenced from:
  "_space_reset", referenced from:
ld: symbol(s) not found for architecture arm64
```

The first attempt stopped on the absent translation unit and was treated as test setup, not behavior evidence. The subsequent failure above establishes the missing public reset implementation.

Green: `make test`, exit 0:

```text
PASS reset isolation and configuration
All core tests passed
```

`make sanitize` passed under AddressSanitizer/UndefinedBehaviorSanitizer; `make analyze` passed Apple Clang static analysis. `git diff --check` passed. Dependency review: only system C headers/libm are used; no third-party dependency has been added or downloaded. No GPU checks or learning ran.
