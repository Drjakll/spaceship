#include "spaceship_bots.h"
#include <string.h>
#include <math.h>
static uint32_t bot_random(uint32_t *rng) {
    uint32_t x = *rng ? *rng : 1;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *rng = x;
}
int space_greedy_target(const SpaceWorld *world, int agent) {
    (void)agent;
    int target = -1;
    for (int e = 0; e < SPACE_MAX_ENEMIES; ++e)
        if (world->enemies[e].phase == 1 && (target < 0 || world->enemies[e].y > world->enemies[target].y)) target = e;
    return target;
}
static SpaceAction aim(const SpaceWorld *world, int agent, int target, float home_x) {
    SpaceAction action = {0};
    if (world->ships[agent].health <= 0) return action;
    float tx = home_x;
    if (target >= 0) {
        const SpaceEnemy *enemy = &world->enemies[target];
        /* Approximate meeting time of accelerated missile and descending target. */
        float distance = fmaxf(0, world->ships[agent].y - 35 - enemy->y);
        float speed = 30 + enemy->vy;
        float dt = (-speed + sqrtf(speed*speed + 600*distance)) / 300;
        tx = enemy->x + enemy->vx * dt;
        while (tx < 25 || tx > 975) tx = tx < 25 ? 50 - tx : 1950 - tx;
    }
    float dx = tx - world->ships[agent].x, dy = 900 - world->ships[agent].y;
    int x = dx > 12 ? 1 : dx < -12 ? -1 : 0;
    int y = dy > 12 ? 1 : dy < -12 ? -1 : 0;
    static const int moves[3][3] = {{SPACE_UP_LEFT,SPACE_UP,SPACE_UP_RIGHT},
        {SPACE_LEFT,SPACE_STAY,SPACE_RIGHT},{SPACE_DOWN_LEFT,SPACE_DOWN,SPACE_DOWN_RIGHT}};
    action.move = moves[y+1][x+1]; action.fire = 1;
    return action;
}
void space_greedy_actions(const SpaceWorld *world, SpaceAction actions[SPACE_MAX_AGENTS]) {
    memset(actions, 0, SPACE_MAX_AGENTS * sizeof(*actions));
    for (int a=0;a<world->config.num_agents;++a)
        actions[a] = aim(world,a,space_greedy_target(world,a),500);
}
static int lane(const SpaceWorld *world, int agent, int *living) {
    int rank = 0; *living = 0;
    for (int a=0; a<world->config.num_agents; ++a) if (world->ships[a].health > 0) {
        ++*living;
        if (a < agent) ++rank;
    }
    return rank;
}
int space_lane_target(const SpaceWorld *world, int agent) {
    if (world->ships[agent].health <= 0) return -1;
    int living; int rank = lane(world, agent, &living), target = -1;
    float low = 1000.0f * rank / living, high = 1000.0f * (rank+1) / living;
    for (int e=0;e<SPACE_MAX_ENEMIES;++e) {
        const SpaceEnemy *enemy = &world->enemies[e];
        if (enemy->phase == 1 && enemy->x >= low && enemy->x < high &&
                (target < 0 || enemy->y > world->enemies[target].y)) target = e;
    }
    return target;
}
void space_lane_actions(const SpaceWorld *world, SpaceAction actions[SPACE_MAX_AGENTS]) {
    memset(actions, 0, SPACE_MAX_AGENTS * sizeof(*actions));
    for (int a=0;a<world->config.num_agents;++a) {
        int living; int rank = lane(world,a,&living);
        if (living) actions[a] = aim(world,a,space_lane_target(world,a),1000.0f*(rank+.5f)/living);
    }
}
void space_random_actions(const SpaceWorld *world, uint32_t *rng, SpaceAction actions[SPACE_MAX_AGENTS]) {
    memset(actions, 0, SPACE_MAX_AGENTS * sizeof(*actions));
    for (int a = 0; a < world->config.num_agents; ++a) {
        actions[a].move = (int)(bot_random(rng) % 9);
        actions[a].fire = (int)(bot_random(rng) % 2);
    }
}
