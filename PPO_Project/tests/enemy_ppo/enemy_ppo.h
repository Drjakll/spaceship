#ifndef ENEMY_PPO_H
#define ENEMY_PPO_H

#include "eval.h"
#include "train.h"

#define ENEMY_OBS_DIM 32
#define ENEMY_VIEW_EPISODE_SECONDS 120

typedef struct {
    const char *run_dir;
    const char *ship_checkpoint;
    uint64_t seed;
    int updates;
    int envs;
    int steps;
    int hidden;
} EnemyTrainConfig;

typedef struct {
    int escaped;
    int killed;
    double seconds;
    double reward;
    int enemies_escaped;
    int enemies_spawned;
    int hits;
    int shots;
    int ammo_remaining;
    int all_ships_out_of_ammo;
    double all_ships_ammo_out_seconds;
    double ship_health_remaining;
    double explosion_damage;
    double damage_taken;
    int ships_lost;
} EnemyEpisode;

typedef struct {
    void *state;
} EnemyPpoPlayer;

int enemy_ppo_observe(const Env *env, uint32_t enemy_id, float obs[ENEMY_OBS_DIM]);
int enemy_ppo_arena_init(Env *env, uint64_t seed, int episode_ticks, uint32_t *target_id);
float enemy_ppo_near_miss_reward(const Enemy *enemy_before, const Missile *missiles_before, int n_missiles_before,
                                 const Enemy *enemy_after, const Missile *missiles_after, int n_missiles_after);
int enemy_ppo_train(const EnemyTrainConfig *cfg);
int enemy_ppo_train_ships(const RunConfig *cfg, const char *enemy_checkpoint);
int enemy_ppo_evaluate(const char *enemy_checkpoint, const char *ship_checkpoint, int episodes, uint64_t seed,
                       const char *output, int random_policy);
int enemy_ppo_player_init(EnemyPpoPlayer *player, const char *enemy_checkpoint, const char *ship_checkpoint,
                         uint64_t seed);
void enemy_ppo_player_free(EnemyPpoPlayer *player);
const Env *enemy_ppo_player_env(const EnemyPpoPlayer *player);
int enemy_ppo_player_step(EnemyPpoPlayer *player);
int enemy_ppo_player_done(const EnemyPpoPlayer *player, int *escaped, int *killed);
void enemy_ppo_player_reset(EnemyPpoPlayer *player);

#endif
