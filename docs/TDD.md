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

## T009 — team reward

Red: `make test`, exit 2: `FAIL tests/test_core.c:238: fabsf(world.reward - (-2.1f)) < 0.00001f`.
Green: `make test`, exit 0: `PASS configurable team reward with no event carry-over` and `All core tests passed`.
Static analysis and whitespace checks passed; no new third-party dependencies. The independently expected simultaneous-event sum is +1 -2 -0.02*5 -1 = -2.1; a following event-free step returns zero. T008 commit: `29d1437`.

## T010 — observation schema

Red: `make test`, exit 2: `"_space_observe", referenced from:` followed by `ld: symbol(s) not found for architecture arm64`.
Green: `make test`, exit 0: `PASS stable normalized observations with complete zero padding` and `All core tests passed`.
Static analysis and whitespace checks passed; no new third-party dependencies. Observation size, masks, agent-relative views, padding, normalization, invalid indices, and read purity are checked. T009 commit: `bd56d72`.

## T011 — bounded storage and stress

Red: `make test`, exit 2: `FAIL tests/test_core.c:288: !space_init(&world, config, 73)` for a NaN reward coefficient.
Green: `make test` and `make sanitize`, exit 0, both reported:

```text
PASS invalid configuration and explicit capacity failures
PASS 1000000 allied decision slots across 22 episodes with deterministic replay
All core tests passed
```

`make analyze` and whitespace checks passed. Tested overflowing deadline configuration, invalid frame skip, finite reward constraints, full missile/enemy arrays, paired seeded traces, bounded observations, and terminal accounting. ASan/UBSan reported no errors. No third-party dependency yet. T010 commit: `fc011a5`.

## T012 — external pinned PufferLib

The first missing-module import was test setup. Red after the public stub existed: `python3 -m unittest discover -s tests -p 'test_setup.py'`, exit 1, `NotImplementedError: External dependency preparation is not implemented` in all three behavior checks.
Green: the same command ran `Ran 3 tests in 0.001s` / `OK`, rejecting internal destinations, corrupt archives, and unverified existing folders.
The verified archive was prepared at `/private/tmp/spaceship-deps/pufferlib-6ffa5b1`. `python3 scripts/check_external.py --pufferlib-root /private/tmp/spaceship-deps/pufferlib-6ffa5b1`, exit 0: `PASS real pinned PufferLib CPU inference smoke`. No upstream source is tracked here.
`make test` passed core plus three Python tests; static analysis and whitespace checks passed. Python syntax compilation passed with `PYTHONPYCACHEPREFIX=build/pycache`; the first attempt hit macOS's external cache permission and was rerun with a local build cache. Dependency-review limits are in `docs/DEPENDENCIES.md`; no CUDA run occurred. T011 commit: `0dee378`.

## T013 — native allied buffers

The first undeclared `dict_free` error was test setup, corrected to upstream `dict_clear`. Red: `clang -std=c11 -O2 -Inative -Itests/stubs -I/private/tmp/spaceship-deps/pufferlib-6ffa5b1/src tests/test_adapter.c native/spaceship_core.c -lm -o build/test_adapter`, exit 1: `Undefined symbols for architecture arm64:` including `_puf_init`, `_puf_reset`, `_puf_step`, `_puf_close`.
Green: `python3 scripts/check_external.py --pufferlib-root /private/tmp/spaceship-deps/pufferlib-6ffa5b1`, exit 0: `PASS real PufferLib buffers route all allies across isolated worlds`. Real upstream Agent/Dict/environment types are used; only the unused graphics header is stubbed for the headless harness. `make test`, `make analyze`, and `git diff --check` passed. Dependencies unchanged from T012 (`803e1b2`).

## T014 — team boundaries

Red: `python3 scripts/check_external.py --pufferlib-root /private/tmp/spaceship-deps/pufferlib-6ffa5b1`, exit 1: `first.boundary_reached == 1`. Green: same command, exit 0: `PASS dead slots and consecutive team boundaries preserve rewards and reset observations`. The test covers an individual inactive slot, two automatic resets, final -2 rewards/terminal flags with initial-state observations, next-step flag clearing, and exact-once logs. Core/Python regression tests, static analysis and whitespace checks passed; dependencies unchanged. T013 commit: `c1f7d86`.

## T025 — lossless reward transport

The public helper first reproduced the inspected upstream [-1,1] transport clamp. Red: `clang -std=c11 -Wall -Wextra -Werror -pedantic -Inative tests/test_ppo.c -lm -o build/test_ppo && ./build/test_ppo`, exit 1: `space_learner_reward(-2) / space_learner_reward(1) == -2`. Green: `make test`, exit 0: `PASS lossless reward transport retains simultaneous events and escape ratio`. Identity scaling preserves raw reward ratios; T027 wires this host/device helper into interior and tail collection. Static/whitespace checks passed, no dependencies added. T014 commit: `6c8fd5e`.

## T026 — complete GAE targets

Red: `make build/test_ppo`, exit 2: undefined `_space_gae` and `_space_gae_delta`. Green: `make test`, exit 0: `PASS independent GAE oracle before/on/after boundaries for every team size`. Hand-computed returns use gamma=0.5, lambda=1, a -2 reward on the rollout tail, bootstrap value 5, a following +3 reward, and true terminal suppression. Consecutive one-step episodes are checked for 1–8 allied slots. The same delta helper will be used by the native patch; this is CPU mathematical evidence, not a CUDA execution claim. Static/whitespace checks passed, dependencies unchanged. T025 commit: `bc4609b`.

## T027 — cooperative native collector patch

Red: `python3 -m unittest discover -s tests -p test_collector.py`, exit 1: `Cooperative collector patch is missing`; `make build/test_ppo`, exit 2: undefined `_space_collector_gae`. Green: `make test`, exit 0, four Python checks and `PASS actual collector incoming-index layout and aliased return storage`. Tests use upstream's incoming-reward indexing and in-place value/return aliasing against manually computed returns. Source-contract checks constrain bootstrap to copied memory without simulation or sampling calls, one policy, all learner rows, explicit final rewards/dones, and removed clipping.
`python3 scripts/check_external.py --pufferlib-root /private/tmp/spaceship-deps/pufferlib-6ffa5b1` verified locked hashes, successfully dry-applied the patch, and passed both real-upstream CPU harnesses. The adapter also compiled/ran under `clang++ -std=c++17`. Static/whitespace checks passed. CUDA compilation/runtime and mutation-free GPU execution remain unverified until the later diagnostic. No PufferLib source is added to this repository. T026 commit: `bb0786e`.

## T015 — bounded native PPO configuration

Red: `python3 -m unittest discover -s tests -p test_config.py`, exit 1: `NotImplementedError: Cooperative PPO configuration is not implemented`. Green: `make test`, exit 0, six Python checks plus C suites. All 1–8 team sizes produce complete worlds/minibatches and a finite four-update budget; incompatible self-play, async, vtrace, row geometry and missing budget are rejected. Static/whitespace checks passed. Chosen native float32/64-wide/two-layer layout is explicitly checked for four-element tensor alignment; upstream PR 691 reports a padding defect for affected layouts, so BF16 is not supported in this initial configuration. Actual optimizer validation remains a CUDA diagnostic. T027 commit: `706242b`.

## T016 — seeded random baseline

Red: `clang -std=c11 -Inative tests/test_bots.c native/spaceship_core.c -lm -o build/test_bots`, exit 1: undefined `_space_random_actions`. Green: `make test`, exit 0: `PASS seeded random baseline reproduces actions and complete traces`. Independent action RNG does not consume world RNG. A 1000-decision paired replay validates legal actions and identical state. Static analysis, regressions and whitespace checks passed; dependencies unchanged. T015 commit: `f2a4d82`.

## T017 — independent greedy baseline

Red: `make build/test_bots`, exit 2: undefined `_space_greedy_actions` and `_space_greedy_target`. Green: `make test`, exit 0: `PASS independent greedy agents pursue the most advanced live threat`. Scripted agents independently pursue the lowest live enemy, excluding explosion hazards, with approximate missile lead and inactive-slot no-ops. Static analysis and whitespace checks passed; dependencies unchanged. T016 commit: `f74ed22`.

## T018 — cooperative lane baseline

Red: `make build/test_bots`, exit 2: undefined `_space_lane_actions` and `_space_lane_target`. Green: `make test`, exit 0: `PASS lane team splits threats and redistributes coverage after an ally dies`. Three separated threats get three distinct defenders; surviving ships repartition lanes after a death. Policy reads do not mutate simulation. Static analysis, regression and whitespace checks passed; no dependencies added. T017 commit: `ae9808e`.

## T032 — original score metric

Added a corrective atomic task for the already-approved original-score requirement before report export. Red: `make build/test_core && ./build/test_core`, exit 1: `world.score == 4`. Green: `make test`, exit 0, verifies a type-1 cooperative kill awards four points exactly once. Score uses original 2/4/8 values and is separate from episode return in native logs. Real-upstream integration, static analysis and whitespace checks passed; dependencies unchanged. T018 commit: `6f51ae2`.

T032 evidence correction: the first ordinary `make test` reused a stale core executable and failed the old score assertion. The commit was made before noticing that result. A subsequent forced `make -B test`, `make analyze`, and real-upstream harness all passed (exit 0). The actual green command was the forced rebuild; no test was weakened.

## T019 — fixed-scenario reports

Red: `make build/evaluate && python3 -m unittest discover -s tests -p test_evaluate.py`, exit 1: `Evaluation reports are not implemented`. Green: `make -B test`, exit 0, seven Python checks plus C suites. Paired runs of all three policies match every semantic metric, conserve enemy obligations, distinguish active decisions from slots, and reconcile reward components/ship kills. Static/whitespace checks passed. Twenty development scenarios (1000–1019) were run for each baseline and for one-ship lanes; reports are in ignored artifacts, summarized in `docs/BASELINES.md`. Reserved test seeds have not been consumed. T032 commit: `329e0a0`.

## T020 — native CPU checkpoint evaluation

Red: `clang -std=c11 -O2 -Inative native/checkpoint.c native/spaceship_core.c -lm -o build/checkpoint && python3 tests/check_checkpoint.py build/checkpoint`, exit 1: `Native checkpoint evaluation is not implemented`. Green: real-upstream check, exit 0: `PASS real PufferNet checkpoint load, recurrent evaluation and truncated-file rejection`. The test constructs an explicitly untrained native-layout artifact, validates the upstream constructor consumed exactly 188800 weights, runs two identical complete seven-enemy episodes through upstream `forward_puffernet`, and rejects a truncated checkpoint before inference. Core/Python regressions, static and whitespace checks passed. Full 200-enemy development evaluation was also launched separately; its quality cannot be described as trained. T019 commit: `40fda41`.

T015 layout detail correction after inspecting the native constructors: the model is bias-free. Its actual tensors are all divisible by eight; no CPU alignment patch is required. `docs/DEPENDENCIES.md` and configuration shape checks now reflect the actual 188800-weight layout.

T020 full development run completed: `python3 scripts/evaluate.py --policy checkpoint --binary build/checkpoint --checkpoint artifacts/untrained-19.bin --output artifacts/dev-untrained.json`, exit 0. Explicitly UNTRAINED initialization seed 19: mean escape 25.95%, defensive failure 45.975%; both targets failed. This validates evaluation execution, not learning.

## T028 — reproducible run manifests

Red: `python3 -m unittest discover -s tests -p test_manifest.py`, exit 1: `NotImplementedError: Run manifest is not implemented`. Green: `make test`, exit 0, nine Python checks plus C suites. Manifest checks cover source/configuration/checkpoint hashes, changed configuration identity, unknown versus measured counters, negative counters and checkpoint labels. Evaluation reports now include manifests; standalone manifest generation was run for the untrained artifact. Static/whitespace checks passed; dependencies unchanged. T020 commit: `e22d44f`.

## T029 — development-only checkpoint selection

Red: `python3 -m unittest discover -s tests -p test_selection.py`, exit 1: `NotImplementedError: Development-only checkpoint selection is not implemented`. Green: `make test`, exit 0, eleven Python checks and C suites. Ranking minimizes defensive failures, then maximizes survivors, then chooses earliest update; an incumbent wins exact ties. Held-out scenarios, mixed rule fingerprints and untrained artifacts are rejected. Content-addressed archives verify bytes against the evaluated hash; latest and selected labels are separate. Static/whitespace checks passed; no actual trained policy has been selected. T028 commit: `7c4d938`.

## T030 — bounded CUDA diagnostic package

Red: `python3 -m unittest discover -s tests -p test_gpu_plan.py`, exit 1: `NotImplementedError: GPU diagnostic plan is not implemented`. Green: `make test`, exit 0, twelve Python checks plus C suites. Receipt validation rejects incomplete, failed, CPU-only and stale-binary matrices. `check_external.py` passed with `PASS GPU diagnostic event fixtures locally for 32 team/boundary combinations`; the same adapter fixtures compiled/ran in C++17.
The new native `space-check` mode collects three real rollouts with controlled +1 kills, -2 escapes, individual deaths and terminal periods 1/63/64/65; it checks independent expected CUDA targets, all-allied ownership, sampled actions/log-probabilities, bootstrap memory/RNG/world preservation, two optimizer updates and a weight round trip. Its NVIDIA execution is NOT performed or claimed on this Mac. The runner requires all 32 actual CUDA results before its bounded train mode; normal training forces diagnostic fixtures off.
`prepare_backend.py` successfully prepared a separate external tree at `/private/tmp/spaceship-deps/spaceship-cuda-prepared` from verified upstream plus our patch; original upstream/reference remain untouched. The diagnostic plan was generated without execution. Static/whitespace checks passed; CUDA compiler, driver, NVML/NCCL and GPU execution remain unavailable here. T029 commit: `e17a6aa`.

## T031 — every-update PPO metrics

Red: `python3 -m unittest discover -s tests -p test_metrics.py`, exit 1: `NotImplementedError: PPO metric parser is not implemented`. Green: `make test`, exit 0, thirteen Python checks plus C suites; focused metric/gate tests also passed after wiring run artifacts. Synthetic fixtures in the native log format verify preservation of losses/KL/entropy/clipping values, update and optimizer counters, active/world decision units, duplicate-update rejection, and inconsistent slot counts. These are parser fixtures, not logs from executed training.
The patch emits every update with native metrics, cumulative raw reward components and measured world/active/physics counters; target summaries explicitly describe the final minibatch or boundary values. The bounded trainer writes a run manifest, retained log, checkpoints and parsed metrics. Real-upstream dry patch/CPU checks, static analysis and whitespace checks passed. Actual CUDA metric emission remains unverified here. T030 commit: `eed6dd4`.

## T021 — original-artwork shared-state viewer

Red: `clang -std=c11 -Inative -I/private/tmp/spaceship-deps/raylib-5.5_macos/include tests/test_view.c native/spaceship_core.c native/spaceship_bots.c /private/tmp/spaceship-deps/raylib-5.5_macos/lib/libraylib.a -framework Cocoa -framework IOKit -framework CoreVideo -framework OpenGL -lm -o build/test_view`, exit 1: undefined `_space_view_open`, `_space_view_draw`, `_space_view_close`.
Green: build via `scripts/build_viewer.py --raylib-root ... --test`, then `./build/test_view`, exit 0: `PASS rendered arena state and subsequent trajectory match headless simulation`. The sandboxed GUI launch could not connect to macOS window services; an approved GUI-capable launch passed. Initial screenshot capture used raylib's filename-only API and landed at the project root; replaced with explicit framebuffer export, verified `build/viewer-smoke.png`, and moved the earlier capture into ignored artifacts.
Visual inspection: three numbered ships, original enemy/missile/explosion artwork, unobstructed counters and HP/contribution panels, clear scripted-policy label. Sixty render calls leave the world bit-identical; the next decision matches headless replay. Main viewer also builds with the real upstream checkpoint runtime. `make test`, real-upstream checks, core/bot/view static analysis and whitespace checks passed. One unused-parameter warning comes from upstream `puffercpu.c`. T031 commit: `da3120e`.

## T022 — manual ship with scripted allies (live keyboard check blocked)

Red: `clang -std=c11 -Inative tests/test_input.c native/spaceship_core.c native/spaceship_bots.c -lm -o build/test_input`, exit 1: undefined `_space_manual_action`. Green: `make test`, exit 0: `PASS keyboard directions with scripted allies and independent cooldown-limited fire`. Opposing keys cancel; a 15-decision joint trace moves the manual ship upward while allies move to defense positions, with exactly two missiles per ship under independent cooldowns. Core/bot/viewer static analysis and whitespace checks passed.
The native viewer was rebuilt with WASD/arrows/space input, optional per-decision traces and a local `build/Spaceship.app` bundle. It launched successfully, but Computer Use returned `Computer Use was not approved to use Spaceship`; no alternative input injection was attempted. The live keyboard smoke remains unverified, so T022 remains unchecked despite implemented controls and passing input/core tests. GUI access was sufficient for the earlier native framebuffer test, but that does not prove keyboard dispatch. T021 commit: `6a70b5d`.

## T033 — standalone viewer build correction

T022 scan correction: core/bot scans passed, but standalone `clang ... --analyze native/viewer.c` returned exit 1: `error: call to undeclared library function 'fmin'`. The preceding task record/commit incorrectly stated that viewer scan passed before its output was inspected. This exposed a missing `<math.h>` include when no PufferLib source was included.
Green after adding the direct include and closing the optional trace on error: `python3 scripts/build_viewer.py --raylib-root /private/tmp/spaceship-deps/raylib-macos-verified` and `clang -std=c11 -Inative -I/private/tmp/spaceship-deps/raylib-macos-verified/include --analyze native/viewer.c -o build/viewer.plist`, both exit 0 without diagnostics. The intermediate analyzer warning about an unclosed optional trace was fixed. Core/Python regression tests and whitespace checks passed. T022 implementation commit: `17057a3`; live keyboard verification remains blocked.

## T023 — local and later-GPU quick-start

The missing-guide check failed first: `python3 -m unittest discover -s tests -p test_quickstart.py`, exit 1: `Local/GPU quick-start guide is missing`. After drafting, link validation correctly failed on `PRD.md`, which was concurrently deleted; its dead link was removed without restoring the user's file. Green: the same focused command, exit 0, `Ran 1 test in 0.034s` / `OK`. All shell blocks parse and local Markdown links resolve. `make test && make analyze` passed with 14 Python tests and all C suites; `git diff --check` passed.

The documented config generator produced 64 arenas, 12288 slots/update and 6 optimizer steps/update; external PufferLib verification succeeded and the 32-case GPU plan reported `executed: false`. The documented lane evaluation completed all 20 development scenarios with zero escapes/failures and three survivors. Earlier fresh dependency extraction, viewer builds and real-upstream CPU checkpoint tests cover those quick-start steps; no CUDA command is claimed executed. No dependencies were introduced by this documentation task.

T022's live keyboard dependency remains blocked by Computer Use access. The rest of the already-authorized documentation was completed independently, with that limitation explicit in the guide. The user confirmed keeping `docs/ENVIRONMENT.md` deleted; README now contains the action/observation schema. Concurrent `PRD.md` deletion, `config/spaceship.ini` team-size edit (3 to 2), and `.DS_Store` changes are preserved outside this task's commit. The committed PRD at `c7333d2` remains the acceptance-review source; no replacement backlog was created. T033 commit: `c7333d2`.

## T034 — evaluation executable and header provenance

Walkthrough found a gap within approved AC-015: the comparison fingerprint omitted the core header and executable used for evaluation. Returned to DEV for this corrective task; the implementation had already been viewed during review. Red: `python3 -m unittest discover -s tests -p test_evaluate.py`, exit 1: `AttributeError: module 'evaluate' has no attribute 'evaluation_identity'`. The new fixtures change a header or binary independently, then verify that the report comparison fingerprint uses the measured identity while semantic episode results remain unchanged.

Green: the focused command passed three tests; `make test` passed all C suites and `Ran 16 tests in 0.280s` / `OK`. `make analyze` and `git diff --check` passed. No dependencies changed. Evaluation reports now retain executable/core-source/core-header hashes, and selection's existing mixed-fingerprint rejection applies to those identities. Older report fingerprints are deliberately incompatible; regenerate candidates together. T023 commit: `97817d7`. This correction does not restore the user-deleted PRD or create a duplicate backlog.
