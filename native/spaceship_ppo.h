#ifndef SPACESHIP_PPO_H
#define SPACESHIP_PPO_H
/* Shared host/device transition contract; no policy-specific reward clipping. */
#ifdef __CUDACC__
#define SPACE_HD __host__ __device__
#else
#define SPACE_HD
#endif
SPACE_HD static inline float space_learner_reward(float reward) { return reward; }
#endif
