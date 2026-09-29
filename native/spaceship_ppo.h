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
#endif
