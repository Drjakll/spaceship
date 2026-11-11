#include <math.h>
#include <stdio.h>

#include "enemy_ppo.h"

int main(void) {
    Enemy before = {.x = 100.0f, .y = 100.0f, .health = 100.0f, .radius = ENEMY_RADIUS};
    Enemy after = {.x = 100.0f, .y = 200.0f, .health = 100.0f, .radius = ENEMY_RADIUS};
    Missile missile_before = {.x = 155.0f, .y = 140.0f, .id = 1};
    Missile missile_after = {.x = 155.0f, .y = 80.0f, .id = 1};
    float near_miss_reward =
        enemy_ppo_near_miss_reward(&before, &missile_before, 1, &after, &missile_after, 1);
    if (near_miss_reward <= 0.0f) {
        fputs("FAIL a close missile miss did not earn reward\n", stderr);
        return 1;
    }
    after.health -= MISSILE_DAMAGE;
    if (enemy_ppo_near_miss_reward(&before, &missile_before, 1, &after, &missile_after, 1) != 0.0f) {
        fputs("FAIL a missile hit earned near-miss reward\n", stderr);
        return 1;
    }

    Env env;
    uint32_t enemy_id = 0;
    if (enemy_ppo_arena_init(&env, 123, EPISODE_TICKS, &enemy_id) != 0) {
        fputs("FAIL could not initialize enemy arena\n", stderr);
        return 1;
    }
    Enemy *enemy = NULL;
    for (int i = 0; i < env.n_enemies; i++) {
        if (env.enemies[i].id == enemy_id) {
            enemy = &env.enemies[i];
            break;
        }
    }
    if (!enemy || env.spawn_timer != ENEMY_SPAWN_TICKS) {
        fputs("FAIL enemy arena did not initialize spawn interval\n", stderr);
        return 1;
    }
    float obs[ENEMY_OBS_DIM];
    if (enemy_ppo_observe(&env, enemy_id, obs) != 0) {
        fputs("FAIL observation unavailable\n", stderr);
        return 1;
    }
    for (int i = 0; i < ENEMY_OBS_DIM; i++) {
        if (!isfinite(obs[i]) || obs[i] < -1.0f || obs[i] > 1.0f) {
            fprintf(stderr, "FAIL observation[%d]=%g is out of bounds\n", i, obs[i]);
            return 1;
        }
    }
    if (enemy_ppo_observe(&env, UINT32_MAX, obs) == 0) {
        fputs("FAIL unknown enemy id was accepted\n", stderr);
        return 1;
    }
    Missile *missile = &env.missiles[env.n_missiles++];
    missile->x = enemy->x;
    missile->y = enemy->y + 80.0f;
    missile->vy = MISSILE_START_VY;
    if (enemy_ppo_observe(&env, enemy_id, obs) != 0 || (obs[20] == 0.0f && obs[21] == 0.0f)) {
        fputs("FAIL nearby missile was not represented\n", stderr);
        return 1;
    }
    env.n_missiles = 0;
    ShipAction idle[NUM_SHIPS] = {{H_STAY, V_STAY, 0}, {H_STAY, V_STAY, 0}, {H_STAY, V_STAY, 0}};
    for (int tick = 0; tick < ENEMY_SPAWN_TICKS - 1; tick++) {
        env_tick(&env, idle);
    }
    if (env.stats.spawned != 1) {
        fputs("FAIL enemy spawned before 0.75 seconds\n", stderr);
        return 1;
    }
    env_tick(&env, idle);
    if (env.stats.spawned != 2 || env.spawn_timer != ENEMY_SPAWN_TICKS) {
        fputs("FAIL enemy did not spawn at 0.75 seconds\n", stderr);
        return 1;
    }
    puts("PASS near misses are rewarded, hits are not, observations are bounded, and enemies spawn every 0.75 seconds");
    return 0;
}
