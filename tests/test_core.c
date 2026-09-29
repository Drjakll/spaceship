#include "spaceship_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(void) {
    test_reset();
    puts("All core tests passed");
    return 0;
}
