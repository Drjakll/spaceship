#include "spaceship.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); exit(1); } } while (0)

typedef struct {
    float observations[SPACE_MAX_AGENTS][SPACE_OBSERVATION_SIZE];
    float actions[SPACE_MAX_AGENTS][2];
    float rewards[SPACE_MAX_AGENTS], terminals[SPACE_MAX_AGENTS];
} Buffers;

static void bind(Env *env, Buffers *buffers) {
    memset(buffers, 0, sizeof(*buffers));
    for (int a = 0; a < env->num_agents; ++a) {
        env->agents[a].observations = buffers->observations[a];
        env->agents[a].actions = buffers->actions[a];
        env->agents[a].rewards = &buffers->rewards[a];
        env->agents[a].terminals = &buffers->terminals[a];
    }
}

int main(void) {
    Env first = {0}, second = {0};
    Buffers a, b;
    Dict kwargs = {0};
    dict_set(&kwargs, "num_agents", 3);
    dict_set(&kwargs, "seed", 73);
    second.rng = 1;
    puf_init(&first, &kwargs);
    puf_init(&second, &kwargs);
    CHECK(first.num_agents == 3 && second.num_agents == 3);
    bind(&first, &a); bind(&second, &b);
    puf_reset(&first); puf_reset(&second);
    CHECK(first.world.rng != second.world.rng);
    CHECK(OBS_SIZE == SPACE_OBSERVATION_SIZE && NUM_ATNS == 2);
    for (int i = 0; i < 3; ++i) CHECK(first.agents[i].policy == 0);
    a.actions[0][0] = SPACE_UP;
    a.actions[1][0] = SPACE_LEFT;
    a.actions[1][1] = 1;
    puf_step(&first);
    CHECK(first.world.ships[0].y == 830 && first.world.ships[1].x == 480);
    CHECK(first.world.projectiles[0].owner == 1 && first.world.projectiles[0].active);
    CHECK(second.world.tick == 0 && second.world.ships[0].y == 850);
    CHECK(a.observations[0][1] == 0.83f);
    for (int i = 0; i < 3; ++i) CHECK(a.rewards[i] == first.world.reward && a.terminals[i] == 0);
    puf_close(&first); puf_close(&second);
    dict_clear(&kwargs);
    puts("PASS real PufferLib buffers route all allies across isolated worlds");
    return 0;
}
