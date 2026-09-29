#pragma once
typedef float obs_t;
#include "pufferenv.h"
#include "spaceship_core.h"
#define OBS_SIZE SPACE_OBSERVATION_SIZE
#define ACT_SIZES {9, 2}
#define NUM_ATNS 2

struct Log {
    float perf, score, killed, escaped, failure_fraction, survivors, episode_length, n;
};
struct Env {
    Log log;
    Agent agents[SPACE_MAX_AGENTS];
    int num_agents, tag, boundary_reached;
    unsigned int rng;
    SpaceWorld world;
    void *client;
};

static double space_option(Dict *kwargs, const char *key, double fallback) {
    return dict_find(kwargs, key) ? dict_get(kwargs, key) : fallback;
}
static int space_int_option(Dict *kwargs, const char *key, int fallback) {
    double value = space_option(kwargs, key, fallback);
    if (!isfinite(value) || value < 0 || value > 2147483647 || value != floor(value)) {
        fprintf(stderr, "Invalid Spaceship option %s\n", key); abort();
    }
    return (int)value;
}
static void space_publish(Env *env) {
    for (int a = 0; a < env->num_agents; ++a) {
        if (!space_observe(&env->world, a, env->agents[a].observations)) abort();
        *env->agents[a].rewards = env->world.reward;
        *env->agents[a].terminals = (float)env->world.terminal;
    }
}
void puf_init(Env *env, Dict *kwargs) {
    SpaceConfig c = space_default_config();
    c.num_agents = space_int_option(kwargs, "num_agents", c.num_agents);
    c.wave_size = space_int_option(kwargs, "wave_size", c.wave_size);
    c.spawn_interval_ticks = space_int_option(kwargs, "spawn_interval_ticks", c.spawn_interval_ticks);
    c.drain_ticks = space_int_option(kwargs, "drain_ticks", c.drain_ticks);
    c.frame_skip = space_int_option(kwargs, "frame_skip", c.frame_skip);
    c.reward_kill = (float)space_option(kwargs, "reward_kill", c.reward_kill);
    c.reward_escape = (float)space_option(kwargs, "reward_escape", c.reward_escape);
    c.reward_damage = (float)space_option(kwargs, "reward_damage", c.reward_damage);
    c.reward_death = (float)space_option(kwargs, "reward_death", c.reward_death);
    uint32_t seed = (uint32_t)space_int_option(kwargs, "seed", 73) ^ (env->rng * 0x9e3779b9u);
    if (!space_init(&env->world, c, seed)) { fprintf(stderr, "Invalid Spaceship configuration\n"); abort(); }
    env->num_agents = c.num_agents;
    for (int a = 0; a < env->num_agents; ++a) env->agents[a].policy = 0;
}
void puf_reset(Env *env) {
    space_reset(&env->world, env->world.rng);
    env->boundary_reached = 0;
    space_publish(env);
}
void puf_step(Env *env) {
    SpaceAction actions[SPACE_MAX_AGENTS] = {0};
    for (int a = 0; a < env->num_agents; ++a) {
        float move = env->agents[a].actions[0], fire = env->agents[a].actions[1];
        if (!isfinite(move) || !isfinite(fire) || move != floorf(move) || fire != floorf(fire) ||
            move < 0 || move > 8 || fire < 0 || fire > 1) {
            fprintf(stderr, "Invalid Spaceship action\n"); abort();
        }
        actions[a].move = (int)move; actions[a].fire = (int)fire;
    }
    if (!space_step(&env->world, actions)) { fprintf(stderr, "Spaceship step failed\n"); abort(); }
    space_publish(env);
}
void puf_render(Env *env) { (void)env; }
void puf_close(Env *env) { (void)env; }
void puf_log(Log *log, Dict *out) {
    dict_set(out, "perf", log->perf);
    dict_set(out, "score", log->score);
    dict_set(out, "killed", log->killed);
    dict_set(out, "escaped", log->escaped);
    dict_set(out, "failure_fraction", log->failure_fraction);
    dict_set(out, "survivors", log->survivors);
    dict_set(out, "episode_length", log->episode_length);
    dict_set(out, "n", log->n);
}
