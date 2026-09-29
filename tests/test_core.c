#include "spaceship_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); exit(1); } } while (0)

static void test_reset(void) {
    SpaceWorld a, b;
    SpaceConfig config = space_default_config();
    CHECK(config.num_agents == 3);
    CHECK(space_init(&a, config, 73));
    CHECK(space_init(&b, config, 73));
    CHECK(memcmp(&a, &b, sizeof(a)) == 0);
    CHECK(a.ships[0].health == 50);
    CHECK(a.ships[0].x < a.ships[1].x);
    CHECK(a.ships[1].x < a.ships[2].x);
    CHECK(a.ships[3].health == 0);
    a.ships[0].health = 0;
    a.tick = 80;
    space_reset(&a, 73);
    CHECK(memcmp(&a, &b, sizeof(a)) == 0);
    space_reset(&a, 74);
    CHECK(a.rng != b.rng);
    CHECK(b.tick == 0 && b.ships[0].health == 50);
    config.num_agents = 0;
    CHECK(!space_init(&a, config, 1));
    config.num_agents = 9;
    CHECK(!space_init(&a, config, 1));
    puts("PASS reset isolation and configuration");
}

static void test_movement(void) {
    SpaceWorld world, saved;
    SpaceConfig config = space_default_config();
    SpaceAction actions[SPACE_MAX_AGENTS] = {{0}};
    for (int n = 1; n <= SPACE_MAX_AGENTS; ++n) {
        config.num_agents = n;
        CHECK(space_init(&world, config, 73));
        for (int i = 0; i < n; ++i) actions[i].move = SPACE_UP;
        CHECK(space_step(&world, actions));
        for (int i = 0; i < n; ++i) CHECK(fabsf(world.ships[i].y - 830) < 0.001f);
        CHECK(world.tick == 4);
    }
    config.num_agents = 3;
    CHECK(space_init(&world, config, 73));
    actions[0].move = SPACE_UP_RIGHT;
    actions[1].move = SPACE_LEFT;
    actions[2].move = SPACE_DOWN;
    world.ships[2].health = 0;
    CHECK(space_step(&world, actions));
    CHECK(fabsf(hypotf(world.ships[0].x - 250, world.ships[0].y - 850) - 20) < 0.001f);
    CHECK(world.ships[1].x == 480);
    CHECK(world.ships[2].y == 850);
    world.ships[0].x = 974;
    CHECK(space_step(&world, actions));
    CHECK(world.ships[0].x == 975);
    saved = world;
    actions[1].move = 9;
    CHECK(!space_step(&world, actions));
    CHECK(memcmp(&world, &saved, sizeof(world)) == 0);
    puts("PASS simultaneous bounded movement for 1-8 ships");
}

static void test_firing(void) {
    SpaceWorld world;
    SpaceConfig config = space_default_config();
    config.frame_skip = 1;
    SpaceAction actions[SPACE_MAX_AGENTS] = {{0}};
    actions[0].fire = actions[1].fire = 1;
    CHECK(space_init(&world, config, 73));
    CHECK(space_step(&world, actions));
    CHECK(world.projectiles[0].active && world.projectiles[0].owner == 0);
    CHECK(world.projectiles[1].active && world.projectiles[1].owner == 1);
    CHECK(world.projectiles[0].x == world.ships[0].x);
    CHECK(world.projectiles[0].y < 850);
    CHECK(world.ships[0].cooldown == 30);
    for (int i = 0; i < 29; ++i) CHECK(space_step(&world, actions));
    CHECK(!world.projectiles[2].active);
    CHECK(space_step(&world, actions));
    CHECK(world.projectiles[2].active && world.projectiles[2].owner == 0);
    CHECK(world.projectiles[3].active && world.projectiles[3].owner == 1);
    world.ships[0].health = 0;
    world.ships[0].cooldown = 0;
    actions[1].fire = 0;
    CHECK(space_step(&world, actions));
    CHECK(!world.projectiles[4].active);
    CHECK(world.ships[1].health == 50);
    world.projectiles[0].y = -11;
    CHECK(space_step(&world, actions));
    CHECK(!world.projectiles[0].active);
    puts("PASS independent firing cooldowns and projectile ownership");
}

static void test_waves(void) {
    SpaceWorld a, b;
    SpaceConfig config = space_default_config();
    config.frame_skip = 1;
    SpaceAction actions[SPACE_MAX_AGENTS] = {{0}};
    CHECK(space_init(&a, config, 73));
    CHECK(space_init(&b, config, 73));
    for (int i = 0; i < 107; ++i) CHECK(space_step(&a, actions));
    CHECK(a.spawned == 0);
    CHECK(space_step(&a, actions));
    CHECK(a.spawned == 1 && a.enemies[0].phase == 1);
    CHECK(a.enemies[0].type >= 0 && a.enemies[0].type < 3);
    CHECK(a.enemies[0].x >= 25 && a.enemies[0].x <= 975);
    for (int i = 0; i < 108; ++i) CHECK(space_step(&b, actions));
    CHECK(memcmp(&a, &b, sizeof(a)) == 0);
    float y = a.enemies[0].y;
    CHECK(space_step(&a, actions));
    CHECK(a.enemies[0].y > y);
    a.enemies[0].x = 974.9f;
    a.enemies[0].vx = 150;
    CHECK(space_step(&a, actions));
    CHECK(a.enemies[0].x <= 975 && a.enemies[0].vx < 0);
    for (int i = 110; i < 324; ++i) CHECK(space_step(&a, actions));
    CHECK(a.spawned == 3);
    puts("PASS deterministic enemy wave schedule and bounded paths");
}

int main(void) {
    test_reset();
    test_movement();
    test_firing();
    test_waves();
    puts("All core tests passed");
    return 0;
}
