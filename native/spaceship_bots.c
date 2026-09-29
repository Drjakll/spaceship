#include "spaceship_bots.h"
#include <string.h>
static uint32_t bot_random(uint32_t *rng) {
    uint32_t x = *rng ? *rng : 1;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *rng = x;
}
void space_random_actions(const SpaceWorld *world, uint32_t *rng, SpaceAction actions[SPACE_MAX_AGENTS]) {
    memset(actions, 0, SPACE_MAX_AGENTS * sizeof(*actions));
    for (int a = 0; a < world->config.num_agents; ++a) {
        actions[a].move = (int)(bot_random(rng) % 9);
        actions[a].fire = (int)(bot_random(rng) % 2);
    }
}
