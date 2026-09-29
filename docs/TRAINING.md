# Cooperative PPO transition design

The three allied ships share one native actor/value network. Separate observations and recurrent state identify each ship; each observes the visible arena, including allies and masked entity slots. Enemies remain scripted simulator entities. All allied slots route to policy index 0, and all rows enter PPO. There is no frozen opponent, learner-role alternation, competitive reward, or self-play pool.

The reference project informed the native C/CUDA integration, synchronous GAE/PPO settings, reproducibility, and boundary checks. Its pursuer/evader row filtering was not copied. The original game's sprites and enemy/weapon parameters are retained alongside deterministic simulation and explicit miss accounting.

## Transition ownership

PufferLib stores `rewards[t]` and `terminals[t]` as the result of the action **before** observation `t`. Therefore action `t` receives reward/terminal at index `t+1`. At the last action in a rollout, the result is still in the live environment buffers; dropping that tail would lose a training example and potentially erase an escape penalty.

The patch passes those final buffers explicitly. The shared host/device helper computes every action's target:

`delta[t] = reward_after_action[t] + gamma*(1-terminal_after_action[t])*next_value[t] - value[t]`

`advantage[t] = delta[t] + gamma*lambda*(1-terminal_after_action[t])*advantage[t+1]`

The terminal mask is the whole-team episode boundary. A dead ship remains an inert slot until that boundary. Its sampled action and log-probability are retained without replacement; all movement/fire labels have the same physical no-op effect for that slot. The observation includes its alive bit and surviving allies. Report active decisions independently of allocated slots.

At a team ending, the native adapter records the final game metrics once, resets the world, exposes initial observations for the next episode, and retains the final action's reward and terminal flags until the next step. The native collector clears recurrent memory before processing the new initial observation. At a continuing rollout cutoff, recurrent state is carried forward.

The boundary value pass copies carried memory into a scratch buffer, applies terminal clearing to that copy, and runs value inference without action sampling. It must not consume action RNG, advance simulation, alter existing actions or mutate the carried state. CPU tests cover indexing and return arithmetic; the NVIDIA diagnostic separately checks those runtime side effects.

## Rewards and PPO variant

Raw team reward is `+1*kill -2*escape -0.02*health_lost -1*new_ship_death`, with configurable coefficients. All health loss is clamped to remaining health. One live enemy can be killed or escape only once. Distinct earlier shooters receive assist metrics; there is no individual last-hit reward.

The adapter uses **identity reward scaling**. The narrow patch removes the upstream training-buffer [-1,1] clamp and keeps both interior and tail rewards intact. Simultaneous events therefore remain additive. Rewards are stored in float32 in the initial training configuration.

The implementation keeps PufferLib's bias-free linear encoder, MinGRU/highway recurrence, combined action/value decoder, Muon optimizer, and native clipped PPO losses. Gamma 0.99, lambda 0.95, clip 0.2, learning rate 0.001, entropy coefficient 0.01, horizon 64 and replay ratio 2 are starting hypotheses. They have not been tuned through Spaceship learning runs. Native advantages are recalculated within the backend's training loop; this is the pinned native implementation, not a separate generic Python PPO trainer.

Every ally uses the same actor/critic information. There is no privileged global critic distinct from the actor, so the project does not claim a MAPPO benchmark reproduction. Good joint return alone is not proof of learned cooperation; evaluate assist/contribution patterns, compare to independent greedy agents, and later substitute partners across training seeds before claiming robustness.

## Geometry and evidence

For `W` worlds, `N` ships and `H` decisions, each rollout has `W*H` world decisions and `W*N*H` allied slots. Actual physics ticks can be fewer than `world_decisions*frame_skip` when an episode ends partway through a held action. Counters are measured in the environment, not inferred from a frame rate.

The default is 64 worlds, 3 ships, horizon 64, minibatch size 4096: 12,288 slots/update and 6 optimizer steps/update at replay ratio 2. Team size changes total agent rows to `64*N`, retaining complete arenas and valid minibatches. The config validator rejects self-play, multiple policies/buffers, async, vtrace and recurrent resets at every horizon.

Evidence levels stay separate:

1. C environment invariants, deterministic replay and sanitizer checks: executed locally.
2. Independent CPU expected returns and incoming-index collector contract: executed locally.
3. Real pinned upstream buffers and full CPU checkpoint inference: executed locally.
4. Native CUDA compilation, real collector targets, optimizer and checkpoint diagnostic: packaged, not executed here.
5. Learning improvement, selected checkpoint, reserved-seed success and trained visual demonstration: not executed.

The GPU diagnostic injects known fixtures only when its explicit diagnostic period is nonzero. It checks boundary periods 1/63/64/65 and all 1–8 team sizes. Normal training forces that option to zero. Its receipt is tied to source and executable hashes. A passing CPU test or source inspection cannot be used as a substitute receipt.
