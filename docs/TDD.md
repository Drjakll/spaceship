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

## T002 — simultaneous movement

Red: `make test`, exit 2: `"_space_step", referenced from:` followed by `ld: symbol(s) not found for architecture arm64`.
Green: `make test`, exit 0: `PASS simultaneous bounded movement for 1-8 ships` and `All core tests passed`.
`make analyze` and `git diff --check` passed. Dependency review: no external libraries added; system libm supplies normalization/bounds. T001 commit: `dbda111`.

## T003 — owned missiles

Red: `make test`, exit 2: `FAIL tests/test_core.c:74: world.projectiles[0].active && world.projectiles[0].owner == 0`.
Green: `make test`, exit 0: `PASS independent firing cooldowns and projectile ownership` and `All core tests passed`.
`make analyze` and `git diff --check` passed; dependency review found no new third-party dependencies. T002 commit: `dbb8beb`.

## T004 — seeded waves

Red: `make test`, exit 2: `FAIL tests/test_core.c:106: a.spawned == 1 && a.enemies[0].phase == 1`.
Green: `make test`, exit 0: `PASS deterministic enemy wave schedule and bounded paths` and `All core tests passed`.
`make analyze` and whitespace checks passed; dependency review found no new external code. T003 commit: `4f56aad`.

## T005 — cooperative damage

Red: `make test`, exit 2: `FAIL tests/test_core.c:135: world.killed == 1 && world.step_kills == 1`.
Green: `make test`, exit 0: `PASS shared damage with single kill and assist attribution` and `All core tests passed`.
Static analysis and whitespace checks passed; no new third-party dependencies. T004 commit: `73873b0`.

## T006 — contact hazards

Red: `make test`, exit 2: `FAIL tests/test_core.c:159: world.ships[0].health == 0 && world.ships[1].health == 40`.
Green: `make test`, exit 0: `PASS per-ship contact damage and finite explosion hazards` and `All core tests passed`.
Static analysis and whitespace checks passed; no new third-party dependencies. T005 commit: `526ccb8`.

## T007 — escaped enemies

Red: `make test`, exit 2: `FAIL tests/test_core.c:185: world.escaped == 1 && world.step_escapes == 1`.
Green: `make test`, exit 0: `PASS exact-once live enemy escape accounting` and `All core tests passed`.
Static analysis and whitespace checks passed; no new external dependencies. An escape occurs when a live enemy fully crosses the bottom (center y > 1025); explosions never count. T006 commit: `6de825e`.

## T008 — finite waves and endings

Red: `make test`, exit 2: `"_space_failure_fraction", referenced from:` followed by `ld: symbol(s) not found for architecture arm64`.
Green: `make test`, exit 0: `PASS episode termination with conserved enemy obligations` and `All core tests passed`.
Static analysis and whitespace checks passed; no third-party dependencies added. Tests cover team defeat, complete waves, deadlines, terminal-state immutability, and scheduled-enemy conservation. T007 commit: `f3be98b`.
