#include "spaceship_ppo.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); } } while (0)
int main(void) {
    CHECK(space_learner_reward(-2) / space_learner_reward(1) == -2);
    CHECK(fabsf(space_learner_reward(-2.1f) + 2.1f) < 1e-6f);
    CHECK(space_learner_reward(0) == 0);
    puts("PASS lossless reward transport retains simultaneous events and escape ratio");
    return 0;
}
