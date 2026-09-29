# Spaceship environment contract

The simulation has 1–8 allied ships (default 3), 64 reusable enemy/hazard slots, and 256 missile slots. Positions are circle centers in a 1000×1000 arena, with y increasing downward. A decision holds each ship's action for four 1/120-second physics ticks by default. All ships choose from the same pre-transition state.

Each action has two categorical heads: movement 0–8 and fire 0–1. Movement order is stay, up, down, left, right, up-left, up-right, down-left, down-right. Directions move at 600 pixels/second, with normalized diagonals and ship centers bounded to [25,975]. Fire repeats on a 30-tick cooldown. Dead ships ignore physical action effects; sampled labels remain intact for PPO likelihood accounting. Missiles retain their owner's identity after that ship dies.

Observations are 2554 float32 values, using the same layout for all supported team sizes. Each ship sees the visible arena with relative positions anchored on itself; actor and value function receive the same observation. RNG state and future spawn samples are excluded. The schema does not imply a separate privileged centralized critic.

| Offset | Count | Fields |
| --- | --- | --- |
| 0 | 10 | Self x/1000, self y/1000, ship index/7, team size/8, time remaining/deadline, ticks to next scheduled spawn/interval, unspawned fraction, killed fraction, escaped fraction, terminal flag. |
| 10 | 8 × 6 | Per ship in index order: presence, alive, relative x/1000, relative y/1000, health/50, firing cooldown/30. |
| 58 | 64 × 19 | Per enemy storage slot: presence, explosion flag, relative x/1000, relative y/1100, horizontal speed/150, downward speed/1000, health/25, three type indicators, explosion lifetime/120, eight per-ship contact cooldowns/168. |
| 1274 | 256 × 5 | Per missile storage slot: presence, relative x/1000, relative y/1100, upward speed/1000, owner index/7. |

Unused slots are all zero on every observation. A dead allied slot remains present with alive=0 and health=0, retaining its position and team context. Entity slots retain their identity until removal, then may be reused; the recurrent policy must use presence changes. Values generated under valid configurations lie in [-1,1]. Observation reads are pure and never consume random numbers.

Per physics tick: move/fire ships, advance missiles, advance the clock/spawn scheduled enemies, move enemies, resolve missile hits, resolve contact/explosion damage, count escapes, then decide episode completion. One missile hits at most one live enemy. Simultaneous candidate collisions use stable slot order; kills precede escapes within a tick. Distinct earlier shooters receive an assist, the killing shooter receives one kill, and overkill damage is clamped to remaining health. Dead enemies become one-second explosion hazards and cannot award more kills. Contact cooldowns are per enemy/ship pair. Projectiles cause no friendly damage.

The raw team reward per decision is `kills - 2*escapes - 0.02*health_lost - ship_deaths`, with configurable coefficients. Event counters clear each decision; health loss is clamped to remaining health. Reward transport into PPO is a separate integration check, so environment rewards alone do not establish learner correctness.

The default wave schedules 200 enemies every 108 physics ticks, then allows 1800 drain ticks, for a 195-second maximum. A live enemy escapes once its center crosses y=1025, fully leaving the arena. Complete team defeat, all scheduled enemies resolved, or the deadline ends the team episode, in that precedence order. Individual death does not end the team episode. These are finite-game outcomes, so they have zero continuation value; rollout boundaries during an ongoing game bootstrap normally.

At termination, scheduled = killed + escaped + unresolved + unspawned. Raw escape fraction is escaped/scheduled. Defensive failure fraction is 1-killed/scheduled and includes obligations remaining on early defeat or deadline. Neither scripted baseline performance nor passing correctness tests establishes trained-policy quality.
