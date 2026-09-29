#include "spaceship_core.h"
#include <string.h>

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
