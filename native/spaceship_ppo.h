#ifndef SPACESHIP_PPO_H
#define SPACESHIP_PPO_H
/* Shared host/device transition contract; no policy-specific reward clipping. */
#ifdef __CUDACC__
#define SPACE_HD __host__ __device__
#else
#define SPACE_HD
#endif
SPACE_HD static inline float space_learner_reward(float reward) { return reward; }
SPACE_HD static inline float space_gae_delta(float reward, float terminal, float value, float next_value, float gamma) {
    return space_learner_reward(reward) + gamma * (1 - terminal) * next_value - value;
}
static inline void space_gae(const float *rewards, const float *terminals, const float *values,
               float bootstrap, int horizon, float gamma, float lambda, float *advantages) {
    float next_adv = 0, next_value = bootstrap;
    for (int t = horizon - 1; t >= 0; --t) {
        float delta = space_gae_delta(rewards[t], terminals[t], values[t], next_value, gamma);
        advantages[t] = delta + gamma * lambda * (1 - terminals[t]) * next_adv;
        next_adv = advantages[t]; next_value = values[t];
    }
}
#ifdef __CUDACC__
#define SPACE_VALUE precision_t
#define SPACE_READ(x) to_float(x)
#define SPACE_WRITE(x) from_float(x)
#else
#define SPACE_VALUE float
#define SPACE_READ(x) (x)
#define SPACE_WRITE(x) (x)
#endif
SPACE_HD static inline void space_collector_gae(const SPACE_VALUE *values, const SPACE_VALUE *incoming_rewards, const SPACE_VALUE *incoming_terminals,
                         float tail_reward, float tail_terminal, float bootstrap, int horizon,
                         float gamma, float lambda, SPACE_VALUE *advantages, SPACE_VALUE *returns) {
    float next_value = bootstrap, next_adv = 0;
    for (int t = horizon - 1; t >= 0; --t) {
        float reward = t == horizon - 1 ? tail_reward : SPACE_READ(incoming_rewards[t + 1]);
        float terminal = t == horizon - 1 ? tail_terminal : SPACE_READ(incoming_terminals[t + 1]);
        float value = SPACE_READ(values[t]);
        float adv = space_gae_delta(reward, terminal, value, next_value, gamma)
            + gamma * lambda * (1 - terminal) * next_adv;
        advantages[t] = SPACE_WRITE(adv);
        returns[t] = SPACE_WRITE(value + adv);
        next_value = value; next_adv = adv;
    }
}
#ifdef __CUDACC__
__global__ void space_advantage(const precision_t *values, const precision_t *rewards,
        const precision_t *terminals, const float *tail_rewards, const float *tail_terminals,
        const precision_t *bootstrap, precision_t *advantages, precision_t *returns,
        float gamma, float lambda, int rows, int horizon) {
    int row = blockIdx.x * blockDim.x + threadIdx.x;
    if (row >= rows) return;
    int off = row * horizon;
    space_collector_gae(values + off, rewards + off, terminals + off,
        tail_rewards[row], tail_terminals[row], to_float(bootstrap[row]), horizon,
        gamma, lambda, advantages + off, returns + off);
}
#endif
#undef SPACE_VALUE
#undef SPACE_READ
#undef SPACE_WRITE
#endif
