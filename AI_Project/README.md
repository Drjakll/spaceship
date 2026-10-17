# Spaceship Q-learning (C)

An independent game written from scratch in C11 and raylib. No original game
headers, scripts, or training data are loaded or modified. Artwork is copied
into `assets/` from your original Pictures folder; the original images are untouched.

## Run

From the repository root:

```sh
cd AI_Project
make
make play
```

Requires a C compiler, `make`, `pkg-config`, and raylib (available on this machine).
Always run the executable from `AI_Project`, so model files resolve locally.

## Train and evaluate

```sh
make test
./build/rl_spaceship --train 10000
./build/rl_spaceship --eval 200
```

Training runs without opening a window. It resumes `models/zigzag_policy.bin` when present
and saves after completion. Evaluation reports mean reward, kills, and damage for
both random movement and the saved policy, including kill rate.
Kill rate is enemies destroyed divided by enemies spawned; missiles still
in flight count as fired. The HUD shows the current episode’s rate. For a fresh experiment, rename or remove
that model file. The binary model is intended for this build/platform.

The viewer loads the saved policy. Controls:

| Key | Action |
| --- | --- |
| T | Toggle live learning (off initially) |
| M | Toggle manual movement |
| Arrow keys | Move in manual mode |
| F | Toggle 1x / 10x simulation speed |
| Space | Pause |
| R | Restart the current episode |
| S | Save policy |
| Escape | Save and quit |

## What the agent learns

Three enemy types spawn at the top, accelerate downward, steer toward the ship,
and evade nearby missiles. With 10-damage missiles, they take three, four, or five hits to destroy.
Destroyed enemies leave 50-pixel-radius explosions that drift downward for 1.2
seconds and damage the ship on contact, with a 0.15-second attack cooldown.
Live enemies also deal contact damage on a cooldown rather than disappearing.
Enemies that escape below the screen incur a small penalty. The ship has 1,000 health points
and fires automatically. Nine actions choose left/stay/right combined with
up/stay/down. Learning therefore controls navigation, not the firing decision.

The discrete state includes the nearest enemy's relative position, the ship's
screen region, threat proximity, weapon cooldown, and whether the threat is a
live enemy or an explosion. Explosion selection accounts for its larger radius. A table stores an expected
discounted return for each state/action pair. After every fixed simulation step:

```text
Q(s,a) += 0.12 * (reward + 0.98 * max Q(next,a) - Q(s,a))
```

Terminal transitions omit the future-value term. Rewards are +0.15 per missile hit, +2 per kill,
-5 per hit, an additional -10 on death, +0.01 per surviving simulation step,
and -0.03 for pushing against a boundary. Escaped enemies cost -0.2. Episodes end on death or after 1,800
steps (60 simulated seconds at 30 steps/second). Training explores randomly
with a probability that decays from 1.0 toward 0.05 over completed episodes.
Watching uses the greedy policy with random tie breaking.

This is actual tabular Q-learning: it learns immediately from transitions and
bootstraps future outcomes, rather than assigning a negative target to a single
snapshot nine seconds earlier. Simulation runs at a fixed step independently
of drawing speed, and uses fixed seeds for repeatable experiments.

Limitations: the state summarizes only one nearby threat, so it does not capture
every threat or bullet. This compact state is partially observable; the policy
is a baseline rather than a guarantee of optimal behavior. Random and learned
evaluations use the same initial seed, but action-dependent random draws and
episode lengths mean their encounters are not exactly paired.

## Files

- `assets/`: local copies of the original game sprites.
- `src/main.c`: environment, learner, viewer, model persistence, and checks.
- `models/zigzag_policy.bin`: saved Q table, created by training (ignored by git).
- `build/rl_spaceship`: executable (ignored by git).

The checks cover contact damage, bullet kills, explosion creation, damage cooldown
and expiry, terminal Q updates, state
bounds, and 100 complete simulations. They do not validate the graphical window.

## Enemy stats matched to your game

| Type | Health | Contact damage | Attack cooldown | Vertical acceleration | Points |
| --- | --- | --- | --- | --- | --- |
| 1 | 25 | 5 | 1.0 s | 90 px/s² | 2 |
| 2 | 37.5 | 7 | 1.2 s | 72 px/s² | 4 |
| 3 | 50 | 10 | 1.4 s | 54 px/s² | 8 |

All types start at 10 px/s downward, use 200 px/s base horizontal steering,
and have a 25 px collision radius. Spawn interval is 0.75 seconds. Explosions
deal 1 damage with a 0.15-second cooldown and begin drifting at 100 px/s,
with -1 px/s² acceleration. Score uses your 2/4/8 point values.
The independent project still uses its own steering/dodge rules, finite explosion
lifetime, arena, episode duration, and automatic firing. The policy is saved
separately because the changed stats require new learning.

Explosions retain the original sprite without fading or an added outline. They
persist until offscreen, inherit the enemy’s remaining attack cooldown, move
with initial velocity (1, 100) px/s and vertical acceleration -1 px/s², and use
the original horizontal reversal rule. Contact uses a 50 px blast radius plus
a 25 px ship radius.

Avoidance update: ship movement now matches 400 px/s. Each step also subtracts
0.15 times the summed proximity risk of all enemies and explosions; risk falls
linearly to zero at 140 px beyond collision range. This explicitly rewards
keeping safe clearance rather than learning only from damage. A new policy file
separates this training from older arena and movement settings.

Combat balance: kills now reward +12, escaped enemies cost -1, proximity risk
costs 0.025 per unit per step. A small bonus (up to +0.04 per step) encourages
aligning below a live enemy at 100–450 px distance when nearby danger is low.
This replaces the stronger avoidance-only weighting above. Damage still costs
-5 per hit, with the existing death penalty. Older policies remain separate.

Zigzag correction: enemies no longer track the ship or dodge missiles. They
spawn at a random x position, then patrol a fixed horizontal span of up to
350 px around that spawn, bounded by the arena. They reverse at the patrol
limits at 200 px/s while accelerating downward. The zigzag policy is trained
separately. Tests check that changing the ship position does not change enemy
movement and that enemies reverse at their patrol limits.

The HUD kill rate resets each episode and displays 0% before any enemy spawns.
Evaluation reports total kills divided by total spawns across all episodes.
