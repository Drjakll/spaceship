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
    first.world.ships[0].health = 0;
    float old_y = first.world.ships[0].y;
    puf_step(&first);
    CHECK(first.world.ships[0].y == old_y && !a.terminals[0]);
    for (int episode = 0; episode < 2; ++episode) {
        first.world.config.wave_size = 1;
        first.world.spawned = 1;
        first.world.enemies[0].phase = 1;
        first.world.enemies[0].health = 10;
        first.world.enemies[0].y = 1026;
        puf_step(&first);
        CHECK(first.boundary_reached == 1);
        CHECK(first.world.tick == 0 && first.world.ships[0].health == 50);
        CHECK(first.log.n == episode + 1 && first.log.escaped == episode + 1);
        for (int i = 0; i < 3; ++i) {
            CHECK(a.terminals[i] == 1 && a.rewards[i] == -2);
            CHECK(a.observations[i][9] == 0 && a.observations[i][1] == 0.85f);
        }
        puf_step(&first);
        CHECK(first.boundary_reached == 0 && a.terminals[0] == 0 && a.rewards[0] == 0);
    }
    Dict logs = {0}; puf_log(&first.log, &logs);
    CHECK(dict_get(&logs, "n") == 2 && dict_get(&logs, "escaped") == 2);
    dict_clear(&logs);
    puf_close(&first); puf_close(&second);
    dict_clear(&kwargs);
    puts("PASS real PufferLib buffers route all allies across isolated worlds");
    puts("PASS dead slots and consecutive team boundaries preserve rewards and reset observations");
    int periods[]={1,63,64,65};
    for(int ships=1;ships<=8;++ships) for(int p=0;p<4;++p) {
        Env env={0}; Buffers buffers; Dict options={0};
        dict_set(&options,"num_agents",ships);dict_set(&options,"diagnostic_period",periods[p]);
        puf_init(&env,&options);bind(&env,&buffers);puf_reset(&env);
        int first_alive=1;
        for(int step=1;step<=192;++step) {
            float expected=(step%67==63?1.f:0.f)-(step%67==64?2.f:0.f);
            int terminal=step%periods[p]==0;
            if(terminal) {expected-=1.02f*(ships-(first_alive?0:1));first_alive=1;}
            else if(step%67==65 && ships>1 && first_alive) {expected-=1.02f;first_alive=0;}
            puf_step(&env);
            for(int a=0;a<ships;++a) {
                CHECK(fabsf(buffers.rewards[a]-expected)<1e-5f);
                CHECK(buffers.terminals[a]==terminal);
            }
        }
        puf_close(&env);dict_clear(&options);
    }
    puts("PASS GPU diagnostic event fixtures locally for 32 team/boundary combinations");
    return 0;
}
