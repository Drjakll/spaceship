# Cooperative Spaceship MARL

Three allied ships share one arena, combine damage, and defend against scripted enemies. Team size is configurable from 1 to 8. The new native C simulation runs independently of graphics; PPO integration uses an **external, pinned PufferLib 5.0 checkout**. There is no self-play, learned enemy policy, or opponent pool in this phase.

Mac simulation, scripted viewing, and native CPU checkpoint evaluation have been executed. CUDA training has **not** run, and no trained policy is included. The development baselines are summarized in [BASELINES.md](docs/BASELINES.md); the untrained checkpoint is only a loader/inference test.

## Run locally

From this repository, with Apple Command Line Tools and Python 3.9+:

```sh
make test
make sanitize
make analyze
make build/evaluate
python3 scripts/evaluate.py --policy lanes --output artifacts/dev-lanes.json
```

The core needs only C11, Make, and libm. Python scripts use the standard library. Tests exercise one million allied decision slots, deterministic replay, all team sizes, collisions, reward accounting, episode boundaries, observation padding, PPO return arithmetic, and evaluation tooling.

To build the Mac viewer, prepare raylib outside this repository. The setup command is for a **new** destination; reuse the resulting directory on later builds.

```sh
SPACE_DEPS=/tmp/spaceship-deps
python3 scripts/prepare_raylib.py --destination "$SPACE_DEPS/raylib-macos"
python3 scripts/build_viewer.py --raylib-root "$SPACE_DEPS/raylib-macos"
./build/viewer
```

On this Mac, the already prepared raylib directory is `/private/tmp/spaceship-deps/raylib-macos-verified`; `build/viewer` and `build/Spaceship.app` are available. The app bundle refers to this checkout's assets, so rebuild after moving the checkout. Temporary dependencies may disappear after OS cleanup; setup can download them again.

```sh
./build/viewer --ships 3 --seed 1000 --policy lanes
./build/viewer --manual 1
```

Manual mode controls the numbered ship with **WASD or arrows**, and **Space** fires. Other ships use the selected script. **P** pauses, **R** starts a new seed, and **Escape** exits. The original `init.c` and assets remain available. The new viewer holds the final episode on screen until reset. Keyboard mapping and joint simulation tests pass; live keyboard dispatch was not verified because Computer Use denied access to the new app.

## RL contract

| Component | Definition |
|---|---|
| Learners | All allied ships share actor/value parameters; each has separate recurrent memory and observations. |
| Enemies | Three deterministic, seeded scripted variants. No enemy trajectories enter PPO. |
| Actions | MultiDiscrete `[9, 2]`: stay/eight movement directions and an independent fire bit. |
| Observations | 2,554 normalized floats: 10 context fields, 8 × 6 ship fields, 64 × 19 enemy fields, 256 × 5 missile fields. Presence/alive masks and stable slot order distinguish padding. |
| Reward | Same team reward for every allied slot: `kills - 2*escapes - 0.02*health_lost - newly_destroyed_ships`. Original 2/4/8 kill score is separate. |
| Timing | 120 fixed physics ticks/s; four ticks per action, or 30 decisions/s. |
| Episode | 200 enemies at 0.9s intervals, then up to 15s drain; ends on team defeat, complete wave, or deadline. |
| Death | A dead ship becomes inert until team reset; its sampled action/log-probability is retained, while physics ignores its actions. |
| Accounting | `killed + escaped + unresolved + unspawned = scheduled` at episode end. |

PPO uses synchronous collection, GAE, clipping, value loss and entropy regularization with PufferLib's native MinGRU/highway model and Muon optimizer. Actor and value head see the same visible arena observation. This is parameter-sharing cooperative PPO, not a claim of reproducing a privileged-critic MAPPO benchmark. See [TRAINING.md](docs/TRAINING.md) for rollout and reward details.

Movement labels are `0 stay, 1 up, 2 down, 3 left, 4 right, 5 up-left, 6 up-right, 7 down-left, 8 down-right`; fire is `0/1`. Coordinates start at the top left, with positive y downward. Observation slots use zero-based indices, stable storage order, and these field orders (`dx/dy` are relative to the observing ship):

| Offset / slots | Fields in order |
|---|---|
| 0 / context | self x/1000, self y/1000, ship ID/7, team size/8, remaining time fraction, next-spawn time fraction, unspawned/wave, killed/wave, escaped/wave, terminal bit |
| 10 / 8 ships × 6 | present, alive, dx/1000, dy/1000, health/50, weapon cooldown/30 |
| 58 / 64 enemies × 19 | present, explosion bit, dx/1000, dy/1100, vx/150, vy/1000, health/25, three type bits, explosion ticks/120, eight per-ship contact cooldowns/168 |
| 1274 / 256 missiles × 5 | present, dx/1000, dy/1100, upward speed/1000, owner ID/7 |

Absent entity slots and unused ship slots are all zero. Dead ships remain present with `alive=0`; their positions stay visible. Relative coordinates and horizontal velocities can be negative. No future enemy type or location is exposed. The constants and implementation live in [spaceship_core.h](native/spaceship_core.h) and [spaceship_core.c](native/spaceship_core.c).

## External PufferLib and CPU checkpoints

PufferLib source, archives, and binaries are not vendored or tracked. Setup rejects destinations inside this repository. The verified revision and source checksums are in [pufferlib-lock.json](pufferlib-lock.json).

```sh
SPACE_DEPS=/tmp/spaceship-deps
python3 scripts/prepare_pufferlib.py --destination "$SPACE_DEPS/pufferlib-6ffa5b1"
python3 scripts/check_external.py --pufferlib-root "$SPACE_DEPS/pufferlib-6ffa5b1"
```

If that destination already exists, use `prepare_pufferlib.py --verify PATH`. An existing matching archive can be supplied with `--archive PATH` to avoid downloading again. The current Mac dependency is `/private/tmp/spaceship-deps/pufferlib-6ffa5b1`.

`check_external.py` verifies the actual upstream files, dry-applies the collector patch, builds the native adapter and CPU runtime, and runs buffer/reset/checkpoint tests. The unused graphics header is stubbed only in the headless adapter harness. The viewer uses real raylib.

The supported checkpoint layout is a bias-free 2554 → 64 encoder, two 64-wide MinGRU layers, and a 12-output decoder (9 movement logits, 2 fire logits, 1 value): **188,800 float32 weights / 755,200 bytes**. Files are checked for exact length and finite values before the upstream loader runs. These are weight snapshots, not exact optimizer-state resumes.

```sh
mkdir -p artifacts
./build/checkpoint --init-untrained artifacts/untrained.bin 19
python3 scripts/evaluate.py --policy checkpoint --binary build/checkpoint \
  --checkpoint artifacts/untrained.bin --checkpoint-kind untrained \
  --output artifacts/dev-untrained.json
python3 scripts/build_viewer.py --raylib-root "$SPACE_DEPS/raylib-macos" \
  --pufferlib-root "$SPACE_DEPS/pufferlib-6ffa5b1"
./build/viewer --checkpoint artifacts/untrained.bin
```

Initialization refuses to overwrite an existing file. This demonstration is explicitly **untrained**. For a later trained snapshot, use its actual path and `--checkpoint-kind trained --checkpoint-update UPDATE` in evaluation. Its architecture must match the supported configuration. CPU action sampling is seeded per scenario; reproducibility is within the same runtime/platform, not promised bit-for-bit between libc implementations or CPU and CUDA.

## Later NVIDIA machine

Initial training target: Linux x86_64 with NVIDIA CUDA. The pinned build expects `nvcc`, a compatible host compiler, Clang, ccache, OpenMP, CUDA libraries, NVML, NCCL, and desktop OpenGL development libraries. Its build script downloads raylib into the external backend. GPU model, VRAM, driver/toolkit compatibility, and compute budget must be checked on that machine. No GPU command below has been executed on this Mac.

Prepare a new external build tree; this does not modify the verified source checkout:

```sh
SPACE_DEPS=/tmp/spaceship-deps
python3 scripts/prepare_backend.py --pufferlib-root "$SPACE_DEPS/pufferlib-6ffa5b1" \
  --destination "$SPACE_DEPS/spaceship-cuda" --build-cuda
python3 scripts/gpu_diagnostic.py plan
python3 scripts/gpu_diagnostic.py check --backend "$SPACE_DEPS/spaceship-cuda" \
  --receipt artifacts/gpu-diagnostic.json --max-seconds 900
python3 scripts/gpu_diagnostic.py train --backend "$SPACE_DEPS/spaceship-cuda" \
  --receipt artifacts/gpu-diagnostic.json --ships 3 --updates 4 --seed 11 \
  --max-seconds 300
```

The diagnostic checks 1–8 ships at terminal periods 1, 63, 64, and 65, with three checked rollouts and two optimizer updates per case. Training refuses missing, incomplete, failed, or stale diagnostic receipts. Normal training explicitly disables diagnostic event injection. Defaults use float32 and CUDA graphs off. Changing source/patch/binary requires preparing, rebuilding, and rerunning the diagnostic.

The four-update smoke contains 49,152 allied action slots, 16,384 world decisions, and 24 optimizer steps for three ships. It is an integration smoke, not a learning-quality experiment. Larger runs remain bounded by both `--updates` and `--max-seconds`. A proposed later cap of roughly 10 million slots per seed would be 813 complete updates (9,990,144 slots) for three ships; agree the hardware/time budget before running that experiment. Seeds 11, 22, and 33 are proposed independent training runs.

The runner generates a consistent configuration from `--ships`, `--updates`, and `--seed`. To inspect those settings without training, generate a separate file:

```sh
python3 scripts/ppo_config.py --ships 3 --updates 4 --seed 11 \
  --output artifacts/three-ship-smoke.ini
```

Changing only `env.num_agents` in an INI keeps the existing total agent count and changes the number of arenas. Use the generator when changing team size to retain 64 arenas and consistent minibatches.

Each training run writes an initial manifest, retained native log, weight snapshots, and—on successful completion—parsed per-update metrics and measured counters. A timeout or failed run retains the initial manifest/log; missing counters remain unknown. The native CUDA collector patch is source-checked and CPU-contract-tested here, but its compilation and actual GPU behavior remain unverified until that diagnostic passes.

## Evaluation and checkpoint selection

Development scenarios are seeds **1000–1019**; reserved evaluation scenarios are **10000–10099**. `--split dev` is the default. Keep the reserved scenarios out of training, reward tuning and checkpoint selection.

```sh
python3 scripts/evaluate.py --policy random --output artifacts/dev-random.json
python3 scripts/evaluate.py --policy greedy --output artifacts/dev-greedy.json
python3 scripts/evaluate.py --policy lanes --ships 1 --output artifacts/dev-one-ship.json
```

Reports contain per-seed contributions, mean/population standard deviation, raw escapes, defensive failures, reward components, survival, duration, action RNG seed, configuration/source/checkpoint hashes, and process-inclusive throughput without rendering. Active decisions are counted separately from all allocated agent slots. No throughput threshold is promised.

**The raw escape target is at most 10%.** Also require at most 10% defensive failures for a successful-defense claim: `(escaped + unresolved + unspawned) / scheduled`. A team dying early cannot pass by simply avoiding later spawns. Both three-ship aiming scripts achieved zero defensive failures on the 20 development scenarios; this does not establish trained-policy quality or learned coordination.

After GPU training, evaluate candidate checkpoints on development seeds with `--checkpoint-kind trained --checkpoint-update UPDATE`, then:

```sh
python3 scripts/select_checkpoint.py --reports artifacts/dev-candidate-1.json \
  artifacts/dev-candidate-2.json --archive artifacts/checkpoints
```

Selection minimizes defensive failures, then maximizes surviving ships, then prefers the earliest update. `--incumbent-report artifacts/checkpoints/selected-report.json` retains the incumbent on exact ties. Archive names are checkpoint SHA-256 hashes; `latest` and `selected` are separate entries in `labels.json`. Mixed rules, untrained policies and held-out reports are rejected. Finally run the frozen selected checkpoint once with `--split test`, and demonstrate that same checkpoint in the Mac viewer. Neither step has been completed for a trained policy yet.

## Verification and project records

- [Red/green test evidence](docs/TDD.md)
- [Dependency identities, licenses and review limits](docs/DEPENDENCIES.md)
- [Initial scripted baseline results](docs/BASELINES.md)
- [PPO transition design and evidence boundaries](docs/TRAINING.md)

For the graphics parity check, build with `python3 scripts/build_viewer.py --raylib-root PATH --test`, then run `./build/test_view` in a GUI-capable terminal. It captures `build/viewer-smoke.png`. Build artifacts, dependencies, reports and checkpoints are ignored; the legacy tracked game binary is left untouched.
