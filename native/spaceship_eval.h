#ifndef SPACESHIP_EVAL_H
#define SPACESHIP_EVAL_H
#include "spaceship_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
typedef void (*SpaceController)(const SpaceWorld *, SpaceAction *, void *);
typedef struct {
    SpaceWorld world;
    long world_decisions, agent_slots, active_decisions;
    double kill_reward, escape_reward, damage_reward, death_reward;
} SpaceEpisode;
static inline int space_parse_int(const char *text, int low, int high) {
    char *end; errno=0; long value=strtol(text,&end,10);
    if(errno || *end || end==text || value<low || value>high) {
        fprintf(stderr,"Invalid integer: %s\n",text); exit(2);
    }
    return (int)value;
}
static inline void space_evaluate(SpaceEpisode *episode, SpaceConfig config, uint32_t seed,
                                  SpaceController controller, void *context) {
    *episode = (SpaceEpisode){0};
    if(!space_init(&episode->world,config,seed)) {fputs("Invalid evaluation configuration\n",stderr);exit(2);}
    while(!episode->world.terminal) {
        SpaceAction actions[SPACE_MAX_AGENTS]={{0}};
        controller(&episode->world,actions,context);
        for(int a=0;a<config.num_agents;++a) episode->active_decisions += episode->world.ships[a].health>0;
        if(!space_step(&episode->world,actions)) {fputs("Simulation failure\n",stderr);exit(2);}
        ++episode->world_decisions; episode->agent_slots += config.num_agents;
        episode->kill_reward += config.reward_kill*episode->world.step_kills;
        episode->escape_reward += config.reward_escape*episode->world.step_escapes;
        episode->damage_reward += config.reward_damage*episode->world.step_damage;
        episode->death_reward += config.reward_death*episode->world.step_deaths;
    }
}
static inline void space_episode_json(const SpaceEpisode *e, uint32_t seed, uint32_t action_seed) {
    const SpaceWorld *w=&e->world;
    int survivors=0; for(int a=0;a<w->config.num_agents;++a) survivors+=w->ships[a].health>0;
    printf("{\"seed\":%u,\"action_seed\":%u,\"scheduled\":%d,\"spawned\":%d,\"killed\":%d,"
        "\"escaped\":%d,\"unresolved\":%d,\"unspawned\":%d,\"score\":%d,"
        "\"failure_fraction\":%.9g,\"escape_fraction\":%.9g,\"survivors\":%d,"
        "\"physics_ticks\":%d,\"duration_seconds\":%.9g,\"world_decisions\":%ld,"
        "\"agent_slots\":%ld,\"active_decisions\":%ld,\"terminal_reason\":%d,\"episode_return\":%.9g,"
        "\"reward_components\":{\"kills\":%.9g,\"escapes\":%.9g,\"damage\":%.9g,\"deaths\":%.9g},\"ships\":[",
        seed,action_seed,w->config.wave_size,w->spawned,w->killed,w->escaped,w->unresolved,w->unspawned,w->score,
        space_failure_fraction(w),w->escaped/(double)w->config.wave_size,survivors,w->tick,w->tick/120.0,
        e->world_decisions,e->agent_slots,e->active_decisions,w->terminal_reason,w->episode_return,
        e->kill_reward,e->escape_reward,e->damage_reward,e->death_reward);
    for(int a=0;a<w->config.num_agents;++a) printf("%s{\"id\":%d,\"health\":%.9g,\"kills\":%d,\"assists\":%d,\"damage\":%.9g}",
        a?",":"",a,w->ships[a].health,w->ships[a].kills,w->ships[a].assists,w->ships[a].damage);
    puts("]}");
}
#endif
