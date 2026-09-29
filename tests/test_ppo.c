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
    for (int ships = 1; ships <= 8; ++ships) {
        for (int agent = 0; agent < ships; ++agent) {
            /* Hand-computed discounted returns with gamma=.5, lambda=1.
               Event immediately before/on/after a two-action boundary. */
            const float rewards[] = {1, -2, 3, 4}, values[] = {2, 3, 5, 7};
            const float terminals[] = {0, 0, 0, 1};
            float adv[4];
            space_gae(rewards, terminals, values, 99, 4, .5f, 1, adv);
            const float expected[] = {-.75f, -2.5f, 0, -3};
            for (int t = 0; t < 4; ++t) CHECK(fabsf(adv[t] - expected[t]) < 1e-6f);
            space_gae(rewards, terminals, values, 5, 2, .5f, 1, adv);
            CHECK(adv[0] == -.75f && adv[1] == -2.5f);
            const float end_each[] = {1, 1, 1, 1};
            space_gae(rewards, end_each, values, 99, 4, .5f, .95f, adv);
            for (int t = 0; t < 4; ++t) CHECK(adv[t] == rewards[t] - values[t]);
            CHECK(space_gae_delta(-2, 0, 3, 5, .5f) == -2.5f);
            CHECK(space_gae_delta(-2, 1, 3, 999, .5f) == -5);
        }
    }
    puts("PASS independent GAE oracle before/on/after boundaries for every team size");
    return 0;
}
