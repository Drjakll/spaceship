# PRD — Cooperative Spaceship RL with PufferLib 5.0

Status: Approved direction for Phase 1 — cooperative PPO against scripted enemies; local implementation authorized
Approval evidence: After reviewing the initial proposal, the user instructed use of PPO with MARL patterned on `~/Developer/puffer-selfplay-modal`, then resolved the training scope: "It will be both. First, let's start with enemies stay scripted." This authorizes starting the cooperative phase; numerical defaults below are adopted implementation defaults, not claims of individually confirmed hyperparameters. Earlier choices remain three ships, at most 10% escape, Mac simulation/CPU evaluation now, NVIDIA training on a friend's computer later.
Authoritative backlog: Section 5 of this document

## 1. Problem and users

Convert the existing single-player C/raylib spaceship game into a reproducible cooperative reinforcement learning environment. Multiple allied ships share one arena, independently choose actions, help destroy enemies, and minimize enemies getting through their defense.

The user needs an environment that can be trained with PufferLib 5.0, measured against simple baselines, and watched with the existing artwork. The research question is whether ships using a shared learned policy can cover the arena effectively without excessive enemy escapes or team deaths.

Repository inspection at commit `94d14d0eee88f25b89cbe71eb020611042573804` found:

- `init.c` runs one keyboard-controlled ship in a 1000 × 1000 raylib window.
- Three enemy variants descend with zigzag motion; the ship fires accelerating upward missiles.
- Simulation, global state, rendering, wall-clock timing, and snapshot recording are coupled.
- Ship movement is frame-dependent; collision damage checks are coupled to the snapshot sampling branch.
- Enemies crossing the bottom are removed without an escape counter or penalty.
- No build instructions, test suite, dependency manifest, or existing PRD/backlog were found; the tracked `test` file is a compiled executable.

## 2. Goals and scope

Goals:

- Run multiple controllable allied ships in each arena, with many independent arenas available to the trainer.
- Encourage cooperation through team rewards, teammate observations, and the ability to combine damage on enemies.
- Make kills, escapes, unresolved enemies, team survival, and per-ship contributions observable.
- Provide deterministic headless simulation, native PufferLib 5.0 integration, reproducible evaluation, and a visual viewer.

Phase 1 implementation scope:

- A C simulation core separated from raylib rendering and keyboard input.
- Three allied ships by default, as selected by the user; proposed configurable team sizes from one through eight.
- Native PufferLib 5.0 environment integration with a pinned upstream revision, configuration, setup/build instructions, training commands, and checkpoint evaluation.
- A shared PPO policy for all allied ships using structured numerical observations, following the reference project's native C/CUDA approach.
- Random, independent greedy, and cooperative lane-coverage baselines.
- Reuse of the existing sprites in a viewer with team metrics; manual control of one ship with scripted allies.
- Automated correctness checks, sanitizer/static-analysis checks, dependency review, and an evidence-based `WALKTHROUGH.md` at handoff.

The user wants an eventual trained checkpoint and demonstration, but explicitly limits current execution to simulation and CPU evaluation on this Mac. The current sprint delivers the environment, scripted demonstrations, checkpoint-evaluation support, and reproducible training instructions for the friend's NVIDIA computer. The later GPU phase must perform bounded training, select a checkpoint using development scenarios, evaluate reserved scenarios, then demonstrate that trained checkpoint on the Mac. Its execution backlog and compute limits will be finalized when GPU specifications/access are available. Scripted or untrained demonstrations cannot satisfy the trained-policy deliverable.

Later experiments may include reward tuning or extended model-quality work within an agreed compute budget.

The user explicitly reiterated: "for now don't do the self play step. just multi agent rl (MARL) step." Current work is cooperative PPO/MARL with scripted enemies only. Learned enemy policies, competitive self-play, and opponent pools are deferred and will require a separate request and scoped backlog. Network multiplayer, pixel observations, a Python/Gym compatibility layer, learned communication messages, a novel training algorithm, and GPU simulation kernels remain outside this build.

## 3. Requirements and constraints

Three default ships, PPO, cooperative training against scripted enemies first, eventual enemy self-play, the 10% escape target, and local simulation/CPU evaluation followed by later NVIDIA training are confirmed user choices. Other numerical settings are configurable initial defaults; empirical tuning remains future work.

**Reference project and transfer limits.** Source inspection of `/Users/avi/Developer/puffer-selfplay-modal` at `91697cc` found a deterministic C core, a native PufferLib adapter, synchronous PPO with GAE, recurrent MinGRU/highway policies, Muon optimization, run manifests, archived checkpoints, fixed-scenario evaluation, and separate latest/selected policies. Its upstream lock pins `6ffa5b10dbbbe4d1e8288367c7d9d3acd3bad4a2`; use that verified revision as the initial integration baseline. Its two-player learner/frozen-opponent routing is specific to Arena Tag and must not be copied into an all-allied learner batch. Reference artifacts, credentials, environment, and source remain untouched.

The reference's tracked `patches/pufferl-arena.patch`, `native/arena_gae.h`, `native/arena_training_contract.h`, `docs/TRAINING_DESIGN.md`, and `docs/learnings/2026-09-19-boundary-reward-fix.md` identify rollout-boundary reward handling as an explicit integration requirement. Its recorded CPU/CUDA checks apply to its own configuration; Spaceship needs independent local checks and later CUDA validation. Build from checksum-verified upstream plus tracked patches, not an unverified generated backend directory.

**PPO/MARL formulation.** All allied ships share actor/value parameters, with separate observations and recurrent memory. All allied slots route to the learning policy; scripted enemies are simulator entities and supply no learner trajectories. The actor and critic use the same current structured observation, including visible teammates and threats; there is no separate privileged-state centralized critic in Phase 1. Describe this precisely as parameter-sharing cooperative PPO rather than claiming a MAPPO paper reproduction. PPO optimizes behavior from the environment's defined states/observations, actions, rewards, and endings.

Use synchronous collection (`base.async=0`), one rollout buffer, GAE (`vtrace=0`), clipped PPO policy loss, value loss, and entropy regularization. Preserve the pinned backend's native policy/optimizer implementation while documenting its differences from vanilla PPO. Start with horizon 64, gamma 0.99, GAE lambda 0.95, clip coefficient 0.2, learning rate 0.001, entropy coefficient 0.01, and replay ratio 2 as configurable reference-derived starting values; they are not validated for Spaceship. Require consistent agent/minibatch geometry. Carry recurrent state across rollout boundaries and reset it at genuine team episode boundaries.

For `W` arenas, `N` ships, and `H` decision steps, a rollout contains `W*H` world transitions and `W*N*H` allied slots, with four physics ticks per decision under the default frame skip. Count living-agent decisions separately. Dead ship slots remain inert team participants until team termination; all action labels have the same no-op effect for that slot, so retain the actually sampled action/log-probability without post-sampling substitution. Shared team returns and the alive indicator continue until the team episode ends. Do not report those inert slots as active decisions. No frozen-enemy rows or two-player half-batch filters belong in the Phase 1 trainer.

**PPO transition correctness.** Store the joint action's resulting reward and terminal outcome exactly once for every collected decision, including the last decision in a rollout. Continuing boundaries bootstrap from the next state; real team endings use zero bootstrap. Value-only boundary inference must not advance simulation, consume action RNG, or mutate carried recurrent memory. Test events immediately before, on, and after collection boundaries, consecutive resets, individual death with surviving allies, and all supported team sizes against independently calculated returns. Document environment tests, CPU target checks, native CPU inference, CUDA target checks, and learning quality as separate evidence levels.

**Reward transport.** The inspected reference backend clips rollout rewards to [-1,1], including its boundary path. Preserve the spaceship reward component ratios throughout the learner path: retain raw game metrics, apply one documented positive linear scale if necessary, and prevent clipping from silently flattening the -2 escape penalty to the same magnitude as a +1 kill. If the pinned backend needs an adapter-specific change, use a small checksum-tracked patch with CPU target fixtures and a later CUDA diagnostic; do not inherit Arena Tag's reward clipping unexamined.

**PufferLib and platforms.** Use the native interface from the upstream `5.0` branch and record an immutable commit plus licenses in the dependency manifest before integration. The upstream native interface exposes per-agent buffers and `puf_init`, `puf_reset`, `puf_step`, `puf_render`, `puf_close`, and `puf_log`. The inspected example uses observation/action metadata in the environment header. Implement against the pinned source, with a real upstream build as the compatibility check.

User constraint: PufferLib is an external dependency, not vendored into this project. Do not commit its source, binaries, archive, or a Git submodule. Accept an explicitly configured external checkout or download a checksum-verified pinned copy into an external cache. Only Spaceship-owned environment/integration code, narrowly scoped compatibility patches if needed, dependency identity/checksums, and setup instructions belong in this repository. Do not mutate the reference project's backend. Reject setup destinations inside the Spaceship repository.

PufferLib 5.0 documents NVIDIA/CUDA training and CPU evaluation/rendering. This workspace is on macOS arm64 with Apple Clang and Make available; raylib headers were not found in the checked standard locations. Simulation tests should need only the C toolchain. The viewer and upstream integration may require additional local build dependencies. Linux/NVIDIA training commands must be provided; a successful training run can only be claimed after execution on available compatible hardware.

Primary references inspected on 2026-09-28:

- [PufferLib documentation](https://puffer.ai/docs.html)
- [5.0 minimal environment](https://github.com/PufferAI/PufferLib/blob/5.0/ocean/minimal/minimal.h)
- [5.0 environment interface](https://github.com/PufferAI/PufferLib/blob/5.0/src/pufferenv.h)
- [5.0 build script](https://github.com/PufferAI/PufferLib/blob/5.0/build.sh)
- [5.0 default configuration](https://github.com/PufferAI/PufferLib/blob/5.0/config/default.ini)
- [PPO paper](https://arxiv.org/abs/1707.06347)
- [Cooperative PPO / MAPPO paper, sections 3.2–3.3](https://arxiv.org/html/2103.01955v4)
- [GAE paper](https://arxiv.org/abs/1506.02438)

The cooperative PPO paper supports studying parameter sharing with common rewards and distinguishes actor/critic information choices. Its benchmark results do not establish Spaceship performance or validate the reference project's optimizer/settings for this task.

**Simulation.** Each arena owns its state, random generator, timers, and bounded entity storage. Reset from the same seed followed by the same joint actions reproduces the same trajectory on the same build. Rendering cannot advance simulation or consume simulation randomness. Use a fixed 1/120-second physics timestep with one policy action held for four ticks, giving 30 decisions per simulated second. Use deterministic spawn scheduling, bounded positions, normalized diagonal movement, validated configuration, and explicit overflow failures; full buffers must never silently discard enemy obligations.

**Game behavior.** Keep the arena, three enemy variants, existing health/damage/cooldown values, missiles, score values, and artwork as starting behavior. Convert the ship's five pixels per nominal 120 Hz frame to 600 pixels/second. Correct timing, collision, boundary, allocation, and repeated-kill defects as part of the conversion. Preserve zigzag flight in a deterministic bounded form rather than reproducing frame-sensitive turning defects. Destroyed enemies remain temporary explosion hazards as in the source, but cannot award additional kills or count as escaped live enemies. Contact damage cooldowns must apply consistently per ship.

**Cooperative agents.** All ships act during the same simulation step. They have separate positions, health, weapon cooldowns, and projectile ownership. Friendly projectiles do not damage allies; allied ships do not block each other. Enemy damage persists across hits by different ships. Log distinct contributing shooters as assists when a kill occurs. No respawn within an episode: a destroyed ship remains an inactive slot with an alive indicator and ignored actions until team reset. The native adapter must represent this lifecycle consistently with the pinned trainer, including recurrent state and terminal handling.

**Actions and observations.** Each ship chooses a movement direction from stay plus eight compass directions, together with a separate binary fire action. Holding fire shoots whenever cooldown allows. Invalid actions must be handled explicitly. Use normalized, fixed-size numerical observations with presence/alive masks, containing self state, identity, teammates, enemies, missiles, and episode progress. Define ordering, coordinate conventions, capacity limits, and padding in a schema. Include no hidden future spawn information. Use a shared policy with distinct per-agent observations; retain bounded padding so supported team sizes use the same observation layout.

**Rewards.** Begin with identical per-step team reward for the participating slots: `(+1.0 × kills) - (2.0 × escapes) - (0.02 × total health lost) - (1.0 × newly destroyed allied ships)`. Calculate health loss after clamping to remaining health, count each event once, and expose component totals. Reward events are cleared on every step. Original score remains a separate metric. No individual last-hit bonus is proposed; every ship benefits from a teammate's kill. Coefficients are configurable, logged, and treated as initial hypotheses rather than demonstrated optimal values.

**Episodes and missed enemies.** Proposed standard scenario: 200 scheduled enemies, one every 0.9 seconds beginning at 0.9 seconds, followed by up to 15 seconds to finish enemies remaining after the final spawn. An episode ends when the entire team is destroyed, the wave is resolved, or the 195-second limit is reached. Record the terminal reason. Raw escapes count live enemies crossing the bottom boundary, exactly once. At the terminal boundary, the 200 enemy obligations partition into killed, escaped, spawned-but-unresolved, and unspawned due to early termination. Define defensive failure fraction as `(escaped + unresolved + unspawned) / scheduled`, so early team death cannot improve the result by reducing the denominator. Report raw escape fraction separately as `escaped / scheduled`. This accounting also makes unresolved enemies at the time limit visible.

**Training and evaluation.** Use the native shared-policy PPO setup above. Save source/upstream/patch identities, environment/reward configuration, seeds, checkpoint hash/path, team size, physics ticks, decision steps, aggregate agent slots, active decisions, PPO updates, optimizer steps, and wall time with each run. Confirm batching divisibility for each team size. Provide bounded smoke settings plus a longer training preset; final rollout sizes depend on available hardware. Python may orchestrate preparation, commands, reports, and artifact checks; simulation and learning remain native C/CUDA.

Maintain immutable archived checkpoints with distinct `latest` and `selected` labels. Select on development scenarios by lowest defensive failure fraction, then highest surviving-ship count, then earliest update; retain the incumbent on exact ties. Use fixed seeded action sampling for the primary evaluation and record its RNG seed; any greedy-action demo is labeled separately. Reserved test scenarios never select a checkpoint. Log PPO losses, entropy, approximate KL, clipping fraction, reward components, and target distributions where the backend exposes them; a low loss is not a gameplay success metric. Compare policies on identical rule/configuration fingerprints. Weight snapshots are not described as exact optimizer-state resume checkpoints. In the later GPU phase, supplement same-policy teams with partner substitutions across independent training seeds before claiming general cooperative robustness.

Evaluate software independently of model quality. Run three baselines on identical scenarios: seeded random legal actions, independent greedy target pursuit, and a scripted lane-coverage team. Compare one-ship performance as an additional reference. Use development scenario seeds 1000–1019 and reserved evaluation seeds 10000–10099. Reserve those evaluation scenarios from training and tuning. If training is requested, propose training seeds 11, 22, and 33, capped initially at 10 million aggregate agent steps per seed, subject to a user-provided wall-time/cost cap; stop at the first applicable limit. Freeze reward/configuration and checkpoint selection using development scenarios before final evaluation.

Report per-seed results and aggregate mean/dispersion for defensive failure fraction, raw escapes, kills, survival, episode duration, per-ship damage/kills/assists, reward components, and simulation throughput with rendering disabled. Compare learned and scripted teams at the same team size. Multi-ship damage fixtures establish the capability to cooperate; observed assist counts and comparison against independent greedy agents provide evidence about learned coordination without treating team reward alone as proof.

Confirmed model-quality target: mean raw escape fraction at most 10% on unseen scenarios. Also report defensive failure fraction so abandoned or unfinished waves do not make a weak policy appear effective; use the stricter 10% defensive-failure threshold for a successful-defense claim. These are experiment targets, not software correctness assertions or promises of convergence. Without executed GPU training, trained-policy quality remains unverified. A short training smoke proves integration, not the quality target.

## 4. Acceptance criteria and validation

| ID | Observable acceptance criterion | How to verify |
| --- | --- | --- |
| AC-001 | A headless core repeats seeded action traces independently of rendering or other arenas. | Replay tests, interleaved-arena tests, render/no-render parity. |
| AC-002 | Team sizes 1–8 support simultaneous movement, per-ship firing, combined damage, safe inactive slots, and no friendly damage. | Action/collision fixtures including two ships contributing to one kill. |
| AC-003 | Each scheduled enemy has exactly one accounting outcome; early team death cannot yield a deceptively low defensive failure fraction. | Kill/escape/timeout/team-death fixtures with exact conservation checks. |
| AC-004 | Reward components match the configured formula once per event with no carry-over. | Exact event-level assertions for team rewards. |
| AC-005 | Actions, observations, normalization, masks, and padding match a documented stable schema. | Boundary/invalid-input checks plus finite-buffer checks across resets. |
| AC-006 | The environment compiles against the pinned real PufferLib 5.0 source and runs multiple independent arenas through its native interface. | Upstream CPU build/evaluation smoke plus a native buffer/lifecycle integration test. |
| AC-007 | Episode completion resets all agent slots coherently without leaking state or losing terminal metrics. | Consecutive-episode and agent-death integration checks. |
| AC-008 | The viewer renders multiple ships with original artwork and accurate team metrics using the same simulation. | Visual inspection, keyboard smoke, state parity test. |
| AC-009 | Baseline evaluation produces reproducible machine-readable results for the documented scenario lists. | Paired reruns with matching semantic metrics; throughput excluded from equality. |
| AC-010 | Setup, build, bounded training, checkpoint loading, and evaluation commands are documented with accurate platform limits. | Fresh setup/build smoke on available hardware; explicitly record unavailable GPU validation. |
| AC-011 | Full tests, supported sanitizers, static analysis, dependency/license review, and a sprint walkthrough have recorded evidence. | Fresh full-suite run, scan logs, marker review, acceptance-criterion mapping. |
| AC-012 | In the later GPU phase, a trained checkpoint has reproducible held-out metrics plus a recorded pass/fail result against the agreed target, followed by a visual demonstration. | Bounded training experiment, fixed-seed checkpoint evaluation, trained-policy viewer inspection. |
| AC-013 | Every allied collected transition has the correct reward/terminal/bootstrap target, including rollout boundaries and dead slots. | Independent CPU expected-return fixtures now; actual collector/CUDA diagnostic before later learning runs. |
| AC-014 | Reward transport preserves configured component ratios into PPO targets. | Single-event and simultaneous-event target fixtures including the -2 escape penalty. |
| AC-015 | Run manifests, PPO metrics, immutable checkpoint identities, and development-only selection are reproducible. | Manifest validation, metric parser fixtures, deterministic checkpoint-ranking fixtures. |

No existing test or audit commands were found. Proposed implementation tooling is a Make-driven C test runner: `make test` for the full automated suite, `make sanitize` for AddressSanitizer/UndefinedBehaviorSanitizer runs, and `make analyze` for compiler/static analysis. These targets do not exist yet. Add a separate real-upstream integration command so missing PufferLib dependencies cannot masquerade as a passing compatibility test. DEV will record exact commands, platform support, dependency vulnerability-review sources, and any limitations. Do not install a Python-specific dependency scanner merely to audit C dependencies.

Local performance checks report measured environment steps/second, aggregate agent steps/second, hardware, compiler flags, and peak memory; no throughput threshold is promised before a baseline exists. Stress simulation for at least one million aggregate agent decisions with bounded storage and sanitizers, without rendering or per-frame snapshot allocation.

## 5. Sprint backlog

Sprint: Cooperative Spaceship RL environment
Review base: `94d14d0eee88f25b89cbe71eb020611042573804`, confirmed before implementation.
Review scope: Project root, excluding generated build artifacts, dependencies, logs, and checkpoints.

This is the authoritative task list. Each task receives its own red/green/refactor evidence and task-specific commit during implementation. Walkthrough evidence belongs beside this list or in the final walkthrough, without a second independent task checklist.

- [x] T001 | P0 | depends: none | AC-001 | Introduce an isolated seeded arena reset; done when: the reset-isolation fixture passes through the new C test runner.
- [x] T002 | P0 | depends: T001 | AC-002 | Implement simultaneous fixed-timestep ship movement; done when: the joint-action movement fixture passes for supported team sizes.
- [x] T003 | P0 | depends: T002 | AC-002 | Implement per-ship missile firing; done when: the projectile ownership/cooldown fixture passes.
- [x] T004 | P0 | depends: T001 | AC-001 | Implement seeded enemy waves; done when: the fixed-seed variant/trajectory fixture matches its expected schedule.
- [x] T005 | P0 | depends: T003, T004 | AC-002 | Resolve missile damage across allied shooters; done when: the combined-damage fixture records exactly one kill with the expected contributors.
- [x] T006 | P0 | depends: T005 | AC-002 | Implement contact hazards; done when: the health/cooldown fixture validates enemy contact plus explosion lifecycle.
- [x] T007 | P0 | depends: T006 | AC-003 | Account for live enemy escapes; done when: the boundary fixture records each escape exactly once.
- [x] T008 | P0 | depends: T007 | AC-003 | Implement wave termination accounting; done when: the terminal fixture partitions all scheduled enemies correctly for every termination reason.
- [x] T009 | P0 | depends: T008 | AC-004 | Emit configurable shared rewards; done when: the event fixture matches every expected reward component across consecutive steps.
- [x] T010 | P0 | depends: T009 | AC-005 | Encode fixed-size per-agent observations; done when: the schema fixture validates normalized values plus inactive-slot padding.
- [x] T011 | P0 | depends: T010 | AC-001 | Enforce bounded simulation storage; done when: the capacity/replay stress suite passes under supported sanitizers.
- [x] T012 | P1 | depends: T011 | AC-006 | Establish the pinned PufferLib 5.0 dependency build; done when: the real upstream CPU smoke builds at the recorded revision.
- [x] T013 | P1 | depends: T012 | AC-006 | Implement native environment buffer integration; done when: a real-upstream harness steps two isolated arenas through the declared interface.
- [x] T014 | P1 | depends: T013 | AC-007 | Implement native team episode boundaries; done when: the lifecycle fixture verifies terminal metrics plus reset observations across consecutive episodes.
- [x] T025 | P1 | depends: T014 | AC-014 | Preserve reward magnitudes in learner transport; done when: the raw-to-target fixture retains the configured kill/escape ratio for interior plus boundary transitions.
- [x] T026 | P1 | depends: T025 | AC-013 | Implement complete rollout target calculations; done when: independent expected-return fixtures pass before/on/after rollout boundaries for allied team slots.
- [x] T027 | P1 | depends: T026 | AC-013 | Prepare the cooperative native collector patch; done when: source-provenance checks plus the local collector contract suite validate all-ally routing with side-effect-free bootstrap inputs.
- [x] T015 | P1 | depends: T027 | AC-010 | Supply bounded shared-policy PPO configuration; done when: native configuration validation accepts every supported team size with a finite step budget.
- [x] T016 | P1 | depends: T010 | AC-009 | Implement a seeded random baseline; done when: its fixed-seed evaluation trace reproduces exactly.
- [x] T017 | P1 | depends: T016 | AC-009 | Implement an independent greedy baseline; done when: its target-selection scenario produces the expected actions.
- [ ] T018 | P1 | depends: T017 | AC-009 | Implement a cooperative lane-coverage baseline; done when: its split-threat scenario demonstrates distinct target coverage.
- [ ] T019 | P1 | depends: T018 | AC-009 | Export fixed-scenario evaluation reports; done when: the baseline report validates against the documented metric schema.
- [ ] T020 | P1 | depends: T015, T019 | AC-010 | Add checkpoint evaluation through the upstream policy runtime; done when: a valid compatible checkpoint produces a complete evaluation report.
- [ ] T028 | P1 | depends: T020 | AC-015 | Record reproducible run manifests; done when: the manifest fixture validates source/configuration/checkpoint identities with explicit step units.
- [ ] T029 | P1 | depends: T028 | AC-015 | Select checkpoints using development scenarios; done when: the ranking fixture retains the expected checkpoint without reading held-out results.
- [ ] T030 | P1 | depends: T029 | AC-013 | Package a bounded GPU correctness diagnostic; done when: the local diagnostic-plan fixture requires complete target cases before enabling a later training command.
- [ ] T031 | P1 | depends: T028 | AC-015 | Parse per-update PPO diagnostics; done when: recorded-log fixtures produce the expected update/optimizer counters with preserved native metrics.
- [ ] T021 | P1 | depends: T018 | AC-008 | Render the shared simulation with existing sprites; done when: the multi-ship viewer passes visual inspection with matching simulation counters.
- [ ] T022 | P1 | depends: T021 | AC-008 | Add manual control with scripted allies; done when: a keyboard smoke demonstrates movement plus cooldown-limited firing alongside allied ships.
- [ ] T023 | P1 | depends: T022, T030, T031 | AC-010 | Document reproducible setup through evaluation; done when: the documented local quick-start completes on available hardware.
- [ ] T024 | P1 | depends: T023 | AC-011 | Produce the evidence-based sprint walkthrough; done when: `WALKTHROUGH.md` maps all criteria to fresh full-suite, scan, dependency-review, or explicit limitation evidence.

T020 may use a temporary initialized compatible policy artifact to test loading when training hardware is unavailable; it must be labeled untrained and cannot satisfy AC-012. Exact checkpoint construction/loading must use the pinned upstream runtime. If that runtime needs unavailable hardware even for the smoke, record the blocker rather than substitute a fabricated success.

AC-012 is an explicitly requested later deliverable with no current execution task because the user chose Mac simulation/CPU evaluation for now. GPU specifications/access and compute limits remain unknown. The 31 tasks above describe the current environment/tooling sprint; completion of this sprint must not be described as completion of the trained-policy objective. AC-013's actual CUDA verification remains deferred even if its CPU checks pass.

## 6. Open questions

The user's instruction to start with scripted enemies authorizes Phase 1. Previously proposed numerical/gameplay settings are adopted as configurable defaults; they remain hypotheses about learnability rather than validated results. Remaining external decisions concern later GPU execution and Phase 2.

| ID | Decision or assumption | Proposed default and impact | Blocks |
| --- | --- | --- | --- |
| Q1 | Number of allied ships. | Resolved: user selected three by default; configurable range 1–8 is adopted from the reviewed proposal. | None. |
| Q2 | Required deliverable. | Resolved: user wants a trained policy demonstrated eventually; current execution is limited to Mac simulation/CPU evaluation per the subsequent platform answer. | None for local implementation. |
| Q3 | Training platform, access, and budget. | Resolved for now: simulation/CPU evaluation on this Mac; later training on a friend's NVIDIA computer. GPU model, operating system, access, and compute limits remain unknown. | Later GPU execution only. |
| Q4 | Meaning of few missed enemies. | Resolved: user selected at most 10% escape on unseen seeds; conservative failure accounting is retained to expose abandoned/unfinished waves. | None. |
| Q5 | Which participants learn. | Resolved: user wants both stages; start with cooperative allied PPO against scripted enemies, then add learned enemies/self-play later. | Phase 2 design only. |
| A1 | Observation, action, and policy design. | Adopted default: structured full-arena numerical observations, movement-plus-fire actions, shared recurrent PPO actor/value parameters, no learned communication. | None. |
| A2 | Cooperation rules. | Adopted default: shared rewards, no friendly damage/blocking, no respawn, fixed slots; allies cooperate through movement, coverage, and shared damage. | None. |
| A3 | Game fidelity and timing. | Adopted default: preserve artwork/enemy variants/combat values, normalize fixed-timestep movement, fix collision/boundary defects, keep explosion hazards, use the finite wave. | None. |
| A4 | Reward coefficients. | Adopted initial formula in section 3 with lossless learner transport; expose configuration and log components; tuning remains empirical. | None. |
| A5 | Episode and evaluation protocol. | Adopted default: 200-enemy waves with a 195-second cap, seeded baselines, separate development/held-out lists, conservative coverage alongside escapes. | None. |
| A6 | Training experiment size if selected. | Three seeds with an initial ceiling of 10 million aggregate agent steps each, additionally bounded by the user's time/cost cap; revise after hardware details. | Actual training. |
| A7 | Interfaces and assets. | Adopted default: native C PufferLib plus raylib viewer, optional Python orchestration; legacy snapshot JSON remains an existing artifact, not imitation-learning data. | None. |
| A8 | Validation and dependency handling. | Adopted default: Make/C tests, immutable upstream pin/checksums, dependency/license review, independent target checks, actual tests before claims. | None. |

Current checkpoint resolution: the user instructed use of PPO/MARL and explicitly said to start with scripted enemies after the earlier PRD preview. Proceed through the local Phase 1 backlog under that direction; reserve GPU execution and enemy-learning design for their later phases. No competitive enemy behavior or cloud deployment is included in the current build.
