#ifndef SPACESHIP_CORE_H
#define SPACESHIP_CORE_H
#include <stdbool.h>
#include <stdint.h>

#define SPACE_MAX_AGENTS 8
#define SPACE_MAX_ENEMIES 64
#define SPACE_MAX_PROJECTILES 256
#define SPACE_WIDTH 1000.0f
#define SPACE_HEIGHT 1000.0f
#define SPACE_DT (1.0f / 120.0f)
#define SPACE_CONTEXT_SIZE 10
#define SPACE_SHIP_FEATURES 6
#define SPACE_ENEMY_FEATURES (11 + SPACE_MAX_AGENTS)
#define SPACE_PROJECTILE_FEATURES 5
#define SPACE_SHIP_OFFSET SPACE_CONTEXT_SIZE
#define SPACE_ENEMY_OFFSET (SPACE_SHIP_OFFSET + SPACE_MAX_AGENTS * SPACE_SHIP_FEATURES)
#define SPACE_PROJECTILE_OFFSET (SPACE_ENEMY_OFFSET + SPACE_MAX_ENEMIES * SPACE_ENEMY_FEATURES)
#define SPACE_OBSERVATION_SIZE (SPACE_PROJECTILE_OFFSET + SPACE_MAX_PROJECTILES * SPACE_PROJECTILE_FEATURES)

typedef struct {
    int num_agents;
    int wave_size;
    int spawn_interval_ticks;
    int drain_ticks;
    int frame_skip;
    float reward_kill, reward_escape, reward_damage, reward_death;
} SpaceConfig;

typedef struct {
    float x, y, health;
    int cooldown;
    int kills, assists;
    float damage;
} SpaceShip;

typedef struct {
    int phase, type, id;
    float x, y, vx, vy, health;
    int contact_cooldown[SPACE_MAX_AGENTS];
    int explosion_ticks;
    unsigned contributors;
} SpaceEnemy;

typedef struct {
    int active, owner;
    float x, y, vy;
} SpaceProjectile;

typedef struct { int move, fire; } SpaceAction;
enum { SPACE_STAY, SPACE_UP, SPACE_DOWN, SPACE_LEFT, SPACE_RIGHT,
       SPACE_UP_LEFT, SPACE_UP_RIGHT, SPACE_DOWN_LEFT, SPACE_DOWN_RIGHT };
enum { SPACE_RUNNING, SPACE_DEFEAT, SPACE_WAVE_COMPLETE, SPACE_DEADLINE, SPACE_ERROR };

typedef struct {
    SpaceConfig config;
    uint32_t rng;
    int tick, spawned, killed, escaped, unresolved, unspawned;
    int terminal, terminal_reason, overflow;
    int step_kills, step_escapes, step_deaths;
    float step_damage, reward;
    double episode_return;
    SpaceShip ships[SPACE_MAX_AGENTS];
    SpaceEnemy enemies[SPACE_MAX_ENEMIES];
    SpaceProjectile projectiles[SPACE_MAX_PROJECTILES];
} SpaceWorld;

SpaceConfig space_default_config(void);
bool space_init(SpaceWorld *world, SpaceConfig config, uint32_t seed);
void space_reset(SpaceWorld *world, uint32_t seed);
bool space_step(SpaceWorld *world, const SpaceAction actions[SPACE_MAX_AGENTS]);
float space_failure_fraction(const SpaceWorld *world);
bool space_observe(const SpaceWorld *world, int agent, float observation[SPACE_OBSERVATION_SIZE]);
#endif
