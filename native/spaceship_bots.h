#ifndef SPACESHIP_BOTS_H
#define SPACESHIP_BOTS_H
#include "spaceship_core.h"
void space_random_actions(const SpaceWorld *world, uint32_t *rng, SpaceAction actions[SPACE_MAX_AGENTS]);
int space_greedy_target(const SpaceWorld *world, int agent);
void space_greedy_actions(const SpaceWorld *world, SpaceAction actions[SPACE_MAX_AGENTS]);
#endif
