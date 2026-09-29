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

int main(void) {
    test_reset();
    test_movement();
    puts("All core tests passed");
    return 0;
}
