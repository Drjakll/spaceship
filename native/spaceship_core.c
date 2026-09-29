#include "spaceship_core.h"
#include <string.h>
#include <math.h>

SpaceConfig space_default_config(void) {
    SpaceConfig config = {0};
    config.num_agents = 3;
    config.wave_size = 200;
    config.spawn_interval_ticks = 108;
    config.drain_ticks = 1800;
    config.frame_skip = 4;
    config.reward_kill = 1.0f;
    config.reward_escape = -2.0f;
    config.reward_damage = -0.02f;
    config.reward_death = -1.0f;
    return config;
}

void space_reset(SpaceWorld *world, uint32_t seed) {
    SpaceConfig config = world->config;
    memset(world, 0, sizeof(*world));
    world->config = config;
    world->rng = seed ? seed : UINT32_C(0x9e3779b9);
    for (int i = 0; i < config.num_agents; ++i) {
        world->ships[i].x = SPACE_WIDTH * (float)(i + 1) / (float)(config.num_agents + 1);
        world->ships[i].y = 850.0f;
        world->ships[i].health = 50.0f;
    }
}

bool space_init(SpaceWorld *world, SpaceConfig config, uint32_t seed) {
    if (!world || config.num_agents < 1 || config.num_agents > SPACE_MAX_AGENTS ||
        config.wave_size < 1 || config.spawn_interval_ticks < 1 ||
        config.drain_ticks < 1 || config.frame_skip < 1) {
        return false;
    }
    world->config = config;
    space_reset(world, seed);
    return true;
}

static bool fire_missile(SpaceWorld *world, int owner) {
    SpaceShip *ship = &world->ships[owner];
    for (int i = 0; i < SPACE_MAX_PROJECTILES; ++i) {
        SpaceProjectile *missile = &world->projectiles[i];
        if (missile->active) continue;
        *missile = (SpaceProjectile){1, owner, ship->x, ship->y, 30};
        ship->cooldown = 30;
        return true;
    }
    world->overflow = 1;
    return false;
}

static uint32_t space_random(SpaceWorld *world) {
    uint32_t value = world->rng;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    world->rng = value;
    return value;
}

static bool spawn_enemy(SpaceWorld *world) {
    static const float health[3] = {10, 15, 25};
    static const float speed[3] = {100, 150, 100};
    for (int i = 0; i < SPACE_MAX_ENEMIES; ++i) {
        SpaceEnemy *enemy = &world->enemies[i];
        if (enemy->phase) continue;
        memset(enemy, 0, sizeof(*enemy));
        enemy->phase = 1;
        enemy->type = (int)(space_random(world) % 3);
        enemy->id = ++world->spawned;
        enemy->x = 25.0f + (float)(space_random(world) % 951);
        enemy->y = -25;
        enemy->vx = speed[enemy->type];
        enemy->vy = 10;
        enemy->health = health[enemy->type];
        return true;
    }
    world->overflow = 1;
    return false;
}

static void move_enemies(SpaceWorld *world) {
    static const float acceleration[3] = {75, 50, 20};
    for (int i = 0; i < SPACE_MAX_ENEMIES; ++i) {
        SpaceEnemy *enemy = &world->enemies[i];
        if (enemy->phase != 1) continue;
        enemy->vy += acceleration[enemy->type] * SPACE_DT;
        enemy->y += enemy->vy * SPACE_DT;
        enemy->x += enemy->vx * SPACE_DT;
        if (enemy->x > 975) { enemy->x = 1950 - enemy->x; enemy->vx = -fabsf(enemy->vx); }
        if (enemy->x < 25) { enemy->x = 50 - enemy->x; enemy->vx = fabsf(enemy->vx); }
    }
}

static void resolve_missiles(SpaceWorld *world) {
    for (int p = 0; p < SPACE_MAX_PROJECTILES; ++p) {
        SpaceProjectile *missile = &world->projectiles[p];
        if (!missile->active) continue;
        for (int e = 0; e < SPACE_MAX_ENEMIES; ++e) {
            SpaceEnemy *enemy = &world->enemies[e];
            if (enemy->phase != 1) continue;
            float dx = enemy->x - missile->x, dy = enemy->y - missile->y;
            if (dx*dx + dy*dy > 35*35) continue;
            float damage = fminf(10, enemy->health);
            enemy->health -= damage;
            enemy->contributors |= 1u << missile->owner;
            world->ships[missile->owner].damage += damage;
            missile->active = 0;
            if (enemy->health <= 0) {
                enemy->phase = 2;
                enemy->explosion_ticks = 120;
                enemy->vx = 0;
                enemy->vy = 50;
                memset(enemy->contact_cooldown, 0, sizeof(enemy->contact_cooldown));
                ++world->killed;
                ++world->step_kills;
                ++world->ships[missile->owner].kills;
                for (int a = 0; a < world->config.num_agents; ++a) {
                    if (a != missile->owner && (enemy->contributors & (1u << a))) ++world->ships[a].assists;
                }
            }
            break;
        }
    }
}

bool space_step(SpaceWorld *world, const SpaceAction actions[SPACE_MAX_AGENTS]) {
    if (!world || !actions || world->terminal) return false;
    for (int i = 0; i < world->config.num_agents; ++i) {
        if (actions[i].move < 0 || actions[i].move > 8 ||
            actions[i].fire < 0 || actions[i].fire > 1) return false;
    }
    static const int dx[9] = {0, 0, 0, -1, 1, -1, 1, -1, 1};
    static const int dy[9] = {0, -1, 1, 0, 0, -1, -1, 1, 1};
    for (int tick = 0; tick < world->config.frame_skip; ++tick) {
        for (int i = 0; i < world->config.num_agents; ++i) {
            SpaceShip *ship = &world->ships[i];
            if (ship->health <= 0) continue;
            int move = actions[i].move;
            float speed = move >= SPACE_UP_LEFT ? 5.0f / sqrtf(2.0f) : 5.0f;
            ship->x = fminf(975, fmaxf(25, ship->x + speed * dx[move]));
            ship->y = fminf(975, fmaxf(25, ship->y + speed * dy[move]));
            if (ship->cooldown > 0) --ship->cooldown;
            if (actions[i].fire && ship->cooldown == 0 && !fire_missile(world, i)) return false;
        }
        for (int i = 0; i < SPACE_MAX_PROJECTILES; ++i) {
            SpaceProjectile *missile = &world->projectiles[i];
            if (!missile->active) continue;
            missile->vy += 300 * SPACE_DT;
            missile->y -= missile->vy * SPACE_DT;
            if (missile->y < -10) missile->active = 0;
        }
        ++world->tick;
        if (world->spawned < world->config.wave_size &&
            world->tick % world->config.spawn_interval_ticks == 0 && !spawn_enemy(world)) return false;
        move_enemies(world);
        resolve_missiles(world);
    }
    return true;
}
