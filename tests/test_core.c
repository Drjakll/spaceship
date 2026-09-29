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

static void test_shared_damage(void) {
    SpaceWorld world;
    SpaceConfig config = space_default_config();
    config.frame_skip = 1;
    SpaceAction actions[SPACE_MAX_AGENTS] = {{0}};
    CHECK(space_init(&world, config, 73));
    world.spawned = 1;
    world.enemies[0] = (SpaceEnemy){.phase=1, .type=1, .id=1, .x=500, .y=500, .health=15};
    world.projectiles[0] = (SpaceProjectile){1, 0, 500, 501, 0};
    world.projectiles[1] = (SpaceProjectile){1, 1, 500, 502, 0};
    world.projectiles[2] = (SpaceProjectile){1, 2, 500, 503, 0};
    CHECK(space_step(&world, actions));
    CHECK(world.killed == 1 && world.step_kills == 1);
    CHECK(world.enemies[0].health == 0 && world.enemies[0].phase == 2);
    CHECK(world.ships[0].damage == 10 && world.ships[1].damage == 5);
    CHECK(world.ships[0].assists == 1 && world.ships[1].kills == 1);
    CHECK(world.ships[2].damage == 0);
    CHECK(!world.projectiles[0].active && !world.projectiles[1].active);
    CHECK(world.projectiles[2].active);
    CHECK(space_step(&world, actions));
    CHECK(world.killed == 1 && world.ships[1].kills == 1);
    puts("PASS shared damage with single kill and assist attribution");
}

static void test_hazards(void) {
    SpaceWorld world;
    SpaceConfig config = space_default_config();
    config.frame_skip = 1;
    SpaceAction actions[SPACE_MAX_AGENTS] = {{0}};
    CHECK(space_init(&world, config, 73));
    world.spawned = 1;
    world.enemies[0] = (SpaceEnemy){.phase=1, .type=2, .x=500, .y=500, .health=25};
    world.ships[0].x = world.ships[1].x = 500;
    world.ships[0].y = world.ships[1].y = 500;
    world.ships[0].health = 4;
    CHECK(space_step(&world, actions));
    CHECK(world.ships[0].health == 0 && world.ships[1].health == 40);
    CHECK(world.step_damage == 14 && world.step_deaths == 1);
    CHECK(space_step(&world, actions));
    CHECK(world.ships[0].health == 0 && world.ships[1].health == 40);
    world.enemies[0].phase = 2;
    world.enemies[0].explosion_ticks = 2;
    memset(world.enemies[0].contact_cooldown, 0, sizeof(world.enemies[0].contact_cooldown));
    CHECK(space_step(&world, actions));
    CHECK(world.ships[1].health == 39);
    CHECK(space_step(&world, actions));
    CHECK(world.enemies[0].phase == 0);
    CHECK(world.escaped == 0);
    puts("PASS per-ship contact damage and finite explosion hazards");
}

static void test_escapes(void) {
    SpaceWorld world;
    SpaceConfig config = space_default_config();
    config.frame_skip = 1;
    SpaceAction actions[SPACE_MAX_AGENTS] = {{0}};
    CHECK(space_init(&world, config, 73));
    world.spawned = 3;
    world.enemies[0] = (SpaceEnemy){.phase=1, .x=250, .y=1024, .vy=240, .health=10};
    world.enemies[1] = (SpaceEnemy){.phase=2, .x=750, .y=1026, .explosion_ticks=2};
    world.enemies[2] = (SpaceEnemy){.phase=1, .x=500, .y=1020, .health=10};
    CHECK(space_step(&world, actions));
    CHECK(world.escaped == 1 && world.step_escapes == 1);
    CHECK(world.enemies[0].phase == 0 && world.enemies[2].phase == 1);
    CHECK(space_step(&world, actions));
    CHECK(world.escaped == 1 && world.enemies[1].phase == 0);
    puts("PASS exact-once live enemy escape accounting");
}

static void test_episodes(void) {
    SpaceWorld world, saved;
    SpaceConfig config = space_default_config();
    config.frame_skip = 1;
    config.wave_size = 3;
    config.drain_ticks = 20;
    SpaceAction actions[SPACE_MAX_AGENTS] = {{0}};
    CHECK(space_init(&world, config, 73));
    world.spawned = 2;
    world.killed = 1;
    world.enemies[0] = (SpaceEnemy){.phase=1, .x=250, .y=100, .health=10};
    for (int a = 0; a < 3; ++a) world.ships[a].health = 0;
    CHECK(space_step(&world, actions));
    CHECK(world.terminal && world.terminal_reason == SPACE_DEFEAT);
    CHECK(world.unresolved == 1 && world.unspawned == 1);
    CHECK(fabsf(space_failure_fraction(&world) - 2.0f/3) < 0.00001f);
    saved = world;
    CHECK(!space_step(&world, actions));
    CHECK(memcmp(&world, &saved, sizeof(world)) == 0);
    space_reset(&world, 73);
    world.spawned = 3; world.killed = 2; world.escaped = 1;
    CHECK(space_step(&world, actions));
    CHECK(world.terminal_reason == SPACE_WAVE_COMPLETE);
    CHECK(world.unresolved == 0 && world.unspawned == 0);
    space_reset(&world, 73);
    world.tick = 343; world.spawned = 3; world.killed = 1; world.escaped = 1;
    world.enemies[0] = (SpaceEnemy){.phase=1, .x=250, .y=100, .health=10};
    CHECK(space_step(&world, actions));
    CHECK(world.terminal_reason == SPACE_DEADLINE);
    CHECK(world.killed + world.escaped + world.unresolved + world.unspawned == 3);
    puts("PASS episode termination with conserved enemy obligations");
}

int main(void) {
    test_reset();
    test_movement();
    test_firing();
    test_waves();
    test_shared_damage();
    test_hazards();
    test_escapes();
    test_episodes();
    puts("All core tests passed");
    return 0;
}
