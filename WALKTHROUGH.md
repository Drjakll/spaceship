# Walkthrough — Cooperative Spaceship MARL

## 1. Delivery summary

The Mac portion is implemented: a deterministic C environment with 1–8 allied ships (three by default), shared rewards, scripted enemies, baseline evaluation, native PufferLib CPU checkpoint inference, and an original-artwork viewer. The external PufferLib 5.0 integration includes cooperative PPO configuration, a collector patch, a bounded NVIDIA diagnostic and training runner. Self-play is disabled.

Delivery is partial relative to the eventual trained-policy objective. CUDA compilation/training, learned-policy quality, reserved-seed evaluation and a trained demonstration have not run. Manual input passes deterministic tests, but live keyboard dispatch remains unverified.

Review base: `94d14d0eee88f25b89cbe71eb020611042573804`. Reviewed implementation head: `a4eb99dfd0ae8b05176f41732d07a11b015a37a9`. Evidence was collected before writing this report. Scope is the sprint's native code, scripts, tests, configuration and documentation. Full diff and zero-context hunks are retained locally in `build/walkthrough-diff.patch` and `build/walkthrough-hunks.patch`.

Concurrent user changes are preserved and excluded from task commits: `.DS_Store`, deleted `PRD.md`, deleted `docs/ENVIRONMENT.md`, and `config/spaceship.ini` changing team size from three to two. The environment-document deletion was explicitly confirmed. Acceptance criteria were recovered read-only with `git show c7333d2:PRD.md`; neither deleted document was restored. The current INI contains 192 agent slots and two ships, hence 96 arenas. The generator and documented `--ships 3` training command still generate the intended three-ship/64-arena setup.

T023 documentation is complete; its T022 live-keyboard dependency is recorded as blocked rather than silently treated as verified. This report completes T024's evidence mapping. T034 fixed executable/header provenance discovered during review, with a separate DEV cycle and commit. No commits were pushed.

## 2. Changes in the diff

All line references below are new/current lines in the captured base-to-working-tree diff. These files were added during the sprint.

| Changed file and diff line(s) | Observed change | Relevant task |
| --- | --- | --- |
| `native/spaceship_core.h`, lines 6–73 | Bounded entities, action labels, state and observation contract | T001–T011 |
| `native/spaceship_core.c`, lines 6–296 | Seeded state, joint physics, cooperative damage, escapes, rewards, obligations and observations | T001–T011, T032 |
| `native/spaceship.h`, lines 1–116 | Real upstream agent buffers, all-allied routing, team autoreset and logs | T013, T014, T025, T030, T031 |
| `native/spaceship_ppo.h`, lines 7–60 | Lossless reward transport and complete incoming-index GAE with explicit tail outcomes | T025, T026 |
| `patches/pufferl-spaceship.patch`, lines 1–138 | Value-only bootstrap, all-row constraints, unclipped rewards, CUDA diagnostic entry and per-update logging | T027, T030, T031 |
| `native/spaceship_check.cuh`, lines 1–102; `native/spaceship_fixture.h`, lines 1–26 | Independent target oracle, controlled boundary fixtures and CUDA assertions | T030 |
| `native/spaceship_native.h`, lines 1–5 | Single-translation-unit bridge for external native builds | T027 |
| `native/spaceship_bots.c`, lines 1–74 | Seeded random, independent greedy and lane-coverage controllers | T016–T018 |
| `native/spaceship_eval.h`, lines 1–54; `native/evaluate.c`, lines 1–25 | Complete episodes and per-seed accounting/contribution records | T019 |
| `native/checkpoint.c`, lines 1–65 | Checked native weight layout and actual upstream CPU policy evaluation | T020 |
| `native/spaceship_view.c`, lines 1–100; `native/viewer.c`, lines 1–84 | Read-only renderer, real-time playback, optional checkpoint/manual controls | T021, T022, T033 |
| `native/spaceship_input.c`, lines 1–7 | Keyboard direction/fire mapping | T022 |
| `scripts/prepare_pufferlib.py`, lines 1–79; `pufferlib-lock.json`, lines 1–26 | Pinned external dependency preparation and source verification | T012 |
| `scripts/check_external.py`, lines 1–37 | Real pinned CPU runtime, checkpoint and adapter checks | T012–T014, T020 |
| `scripts/prepare_backend.py`, lines 1–39 | Separate external CUDA integration tree | T027, T030 |
| `scripts/prepare_raylib.py`, lines 1–32; `scripts/build_viewer.py`, lines 1–38 | External raylib archive verification and standalone/checkpoint viewer builds | T021, T033 |
| `scripts/ppo_config.py`, lines 1–68; `config/spaceship.ini`, lines 1–46 | Bounded shared-policy geometry; current INI includes the preserved user edit | T015 |
| `scripts/evaluate.py`, lines 1–66 | Fixed scenario reports with rule, executable, header and source identities | T019, T028, T034 |
| `scripts/run_manifest.py`, lines 1–49; `scripts/select_checkpoint.py`, lines 1–56 | Explicit work units, immutable checkpoint archive, development-only ranking | T028, T029 |
| `scripts/gpu_diagnostic.py`, lines 1–86; `scripts/ppo_metrics.py`, lines 1–32 | Required CUDA receipt, bounded training and native metric parsing | T030, T031 |
| `tests/test_core.c`, lines 1–360; `tests/test_adapter.c`, lines 1–96; `tests/test_ppo.c`, lines 1–39 | Simulation invariants, actual buffer lifecycle and independent return fixtures | T001–T014, T025–T027, T030 |
| `tests/test_evaluate.py`, lines 1–55 | Reproducible metrics and executable/header comparison identity regression | T019, T034 |
| `Makefile`, lines 1–36 | C/Python tests, sanitizer and static-analysis targets | T001, T011 |
| `README.md`, lines 1–158; `docs/TRAINING.md`, lines 1–47 | Setup, complete observation/action schema, PPO semantics and execution limits | T023 |
| `docs/TDD.md`, lines 1–206; `docs/DEPENDENCIES.md`, lines 1–27; `docs/BASELINES.md`, lines 1–14 | Test-first evidence, dependency review and measured baseline results | Sprint evidence |

## 3. Verification evidence

Platform: macOS arm64, Apple Clang 21.0.0 (`clang-2100.3.34.2`). The core is C11, built with `-Wall -Wextra -Werror -pedantic -O2`. Python tooling uses the standard library.

Fresh full suite after the final implementation correction: `make test`, exit **0**. Earlier in this review, `make -B test` also forced recompilation successfully. Final verbatim summaries from `build/walkthrough-tests.log`:

```text
PASS reset isolation and configuration
PASS simultaneous bounded movement for 1-8 ships
PASS independent firing cooldowns and projectile ownership
PASS deterministic enemy wave schedule and bounded paths
PASS shared damage with single kill and assist attribution
PASS per-ship contact damage and finite explosion hazards
PASS exact-once live enemy escape accounting
PASS episode termination with conserved enemy obligations
PASS configurable team reward with no event carry-over
PASS stable normalized observations with complete zero padding
PASS invalid configuration and explicit capacity failures
PASS 1000000 allied decision slots across 22 episodes with deterministic replay
All core tests passed
PASS lossless reward transport retains simultaneous events and escape ratio
PASS independent GAE oracle before/on/after boundaries for every team size
PASS actual collector incoming-index layout and aliased return storage
PASS seeded random baseline reproduces actions and complete traces
PASS independent greedy agents pursue the most advanced live threat
PASS lane team splits threats and redistributes coverage after an ally dies
PASS keyboard directions with scripted allies and independent cooldown-limited fire
Ran 16 tests in 0.283s

OK
```

`make sanitize`, exit **0**, rebuilt the core with AddressSanitizer and UndefinedBehaviorSanitizer; no diagnostics were emitted. Verbatim final lines from `build/walkthrough-sanitize.log`:

```text
PASS 1000000 allied decision slots across 22 episodes with deterministic replay
All core tests passed
```

`python3 scripts/check_external.py --pufferlib-root /private/tmp/spaceship-deps/pufferlib-6ffa5b1`, exit **0**. It checks locked upstream file hashes, dry-applies the patch, and compiles/runs the real upstream CPU runtime and the Spaceship adapter. Verbatim summaries from `build/walkthrough-external.log`:

```text
PASS real pinned PufferLib CPU inference smoke
Created UNTRAINED initialization; no PPO updates performed
PASS real PufferNet checkpoint load, recurrent evaluation and truncated-file rejection
PASS real PufferLib buffers route all allies across isolated worlds
PASS dead slots and consecutive team boundaries preserve rewards and reset observations
PASS GPU diagnostic event fixtures locally for 32 team/boundary combinations
```

The last line describes CPU execution of controlled fixtures; it is not a CUDA result. The unused graphics include is stubbed only for the headless adapter harness, not for policy math or the viewer.

`python3 scripts/build_viewer.py --raylib-root /private/tmp/spaceship-deps/raylib-macos-verified --test`, followed by `./build/test_view` in an approved GUI-capable execution, both exited **0**. Verbatim summary:

```text
PASS rendered arena state and subsequent trajectory match headless simulation
```

The test checks 60 render calls against an unchanged world, then compares the next joint step with headless replay. `build/viewer-smoke.png` was visually inspected: three numbered ships, original sprites, 31 kills, zero escapes, 33 spawned, score 144, return +31, health and assists shown at 30 simulated seconds.

Both standalone and real-PufferLib viewer builds succeeded. Final build includes native CPU checkpoint support. `./build/viewer --checkpoint artifacts/untrained-19.bin --frames 20` exited **0**; summary: none found (the successful short GUI run emitted no output). It used an explicitly untrained artifact.

Static checks: `make analyze`, separate Clang analyzer runs for `native/viewer.c` and `native/spaceship_view.c` with the external raylib include, and `git diff --check` all exited **0** without diagnostics. The checkpoint-enabled viewer compile emits one upstream `puffercpu.c` unused-parameter warning for `num_embeddings`; the standalone build is clean.

Dependency review: the external PufferLib archive/native files match the lock, MIT license checked; raylib's official macOS archive matches its SHA-256, zlib license checked. Third-party sources were not committed. A generated upstream patch working copy was moved from ignored build storage to the external dependency directory. Preparation of `/private/tmp/spaceship-deps/spaceship-cuda-final` from verified source plus the tracked patch succeeded; this is an uncompiled external build tree. Vulnerability-review sources and limits are recorded in `docs/DEPENDENCIES.md`.

The final scoped marker search over whole changed files used `rg -n --hidden --no-ignore 'TODO|FIXME|HACK|@pytest\.mark\.skip' -- <changed-files>`: exit **1**, no matches. The generated report itself was written afterward. Deleted PRD/schema documents were not present at the review base, so they contribute no removed-only marker content to this base-to-working-tree diff.

Model evaluation is separate from software testing. `python3 scripts/evaluate.py --policy lanes --output artifacts/final-dev-lanes.json`, exit **0**, ran all 20 development seeds (1000–1019), 200 enemies each: mean raw escapes **0%**, defensive failures **0%**, kills **200**, survivors **3**. This is a script, not a learned policy. Earlier random/greedy/single-ship baselines are in `docs/BASELINES.md`; the real native untrained checkpoint's completed development run had **25.95%** raw escapes and **45.975%** defensive failures. Reserved seeds 10000–10099 have not been used for training, tuning or selection.

The final lane report measured about **246,006 world decisions/s** and **738,019 allied slots/s** across the development suite, including process startup, with rendering disabled. `/usr/bin/time -l ./build/evaluate lanes 3 1000 200` completed one scenario with **1,703,936 bytes maximum resident set size** and **1,229,136 bytes peak memory footprint**. Its initial sandboxed resource query failed (`sysctl kern.clockrate: Operation not permitted`); the approved retry exited 0 and produced these values. These are local headless script measurements, not policy-inference or GPU throughput guarantees.

## 4. Acceptance criteria

| Criterion | Status | Diff or execution evidence |
| --- | --- | --- |
| AC-001 — independent seeded core and render parity | met | Interleaved worlds, million-slot replay, actual read-only renderer and next-step parity |
| AC-002 — 1–8 ships with joint actions and cooperation | met | Movement, weapon ownership/cooldown, shared damage/assist and inactive-slot fixtures |
| AC-003 — conserved enemy obligations | met | Escape, wave, defeat, deadline, capacity and complete-episode assertions |
| AC-004 — exact team reward events | met | Simultaneous events, clamped damage, custom coefficients and no carry-over; original score separate |
| AC-005 — stable documented schema | met | README field ordering/normalization and core observation/action checks |
| AC-006 — pinned upstream native integration | met | Actual locked CPU implementation and two-world native buffer harness |
| AC-007 — coherent team reset | met | Consecutive boundaries retain final rewards/metrics and publish reset observations |
| AC-008 — accurate viewer and keyboard smoke | unverified | Rendering/metrics/parity passed; live keyboard access was denied |
| AC-009 — reproducible baseline reports | met | Paired semantic records and completed development reports |
| AC-010 — setup, bounded commands, checkpoint loading, platform limits | met | External setup/build/CPU runs and guide with explicit unrun CUDA boundary |
| AC-011 — tests, sanitizers, scans, review and walkthrough | met | Fresh suite and recorded scan/dependency evidence above, with gaps below |
| AC-012 — trained held-out evaluation and demonstration | unverified | Deferred by the current Mac-only execution scope; no trained checkpoint exists |
| AC-013 — complete actual learner targets | unverified | Independent CPU oracle and native fixtures pass; real CUDA collector execution is pending |
| AC-014 — reward ratios into PPO targets | unverified | Raw/CPU targets preserve ratios and patch removes clipping; actual CUDA transport pending |
| AC-015 — manifests, actual PPO metrics and checkpoint selection | unverified | Local manifest/parser/ranking/provenance fixtures pass; actual Spaceship training logs and trained selection pending |

## 5. Limitations

- **AC-008 / T022:** Computer Use returned `Computer Use was not approved to use Spaceship`. No alternate keyboard injection was attempted. Automated input mapping/joint physics and GUI rendering pass, but a user keyboard smoke with WASD/arrows and Space is still needed.
- **AC-012:** No NVIDIA training, learned-policy improvement, held-out target result, trained checkpoint selection, or trained demonstration has occurred. The <=10% target is not claimed for a learned policy. The GPU's model, operating system/toolchain, VRAM and agreed time budget remain unknown.
- **AC-013:** CUDA compilation, actual collector target comparisons, action/log-probability checks, bootstrap side effects, optimizer updates and checkpoint round trip are packaged but unexecuted. The runner refuses ordinary training until all 32 actual CUDA diagnostic cases pass with matching identities. CPU/source checks are not substitutes.
- **AC-014:** Unclipped reward transport has local mathematical/adapter evidence but not actual Spaceship CUDA execution evidence.
- **AC-015:** Per-update metric tests use synthetic native-format records; candidate-ranking tests use fixtures. No actual trained Spaceship checkpoint or CUDA logs exist yet. Weight snapshots omit optimizer state and do not support exact optimizer-state resume. Old evaluation reports must be regenerated together to use T034's executable/header fingerprint.
- The full C/Python suite is separate from dependency and GUI checks. The sanitizer target instruments the simulation core, not the prebuilt raylib archive or CUDA backend. Comprehensive native dependency vulnerability scanning was unavailable; raylib advisory applicability and the upstream Muon issue are documented in `docs/DEPENDENCIES.md`. The selected bias-free float32 shape avoids the identified padding condition, but requires the GPU optimizer diagnostic.
- `--warmup` in the viewer advances using the lane script even when another playback policy is selected. Use its default zero for checkpoint demonstrations. Headless checkpoint evaluation, not a reset within an interactive session, is the reproducibility reference; interactive reset does not reseed policy action RNG.
- Comparison fingerprints now bind reports to executable and physics source/header hashes. Source hashes record the current checkout; rebuilding remains necessary after code changes. Cross-platform bit-identical action sampling is not promised.
- Three-ship greedy and lane scripts both solve the development scenarios; those results do not establish learned cooperation or prove a unique advantage of lane allocation. Hyperparameters/reward coefficients remain untuned initial choices.
- User deletions and the two-ship INI edit remain uncommitted and preserved. The historical approved PRD supplies acceptance criteria; this report does not claim its old task checkboxes are a current independent backlog. T022's unmet check is carried explicitly; the remaining current implementation/documentation tasks have evidence above.
