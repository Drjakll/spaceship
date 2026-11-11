#define _POSIX_C_SOURCE 200809L

#include "enemy_ppo.h"

#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "nn.h"
#include "ppo.h"

#define ENEMY_ACTIONS 3
#define ENEMY_MISSILE_SLOTS 4
#define ENEMY_MISSILE_FEATURES 3
#define ENEMY_TYPE_FEATURES 3
#define ENEMY_EPISODE_REWARD 3.0f
#define ENEMY_DEATH_REWARD (-3.0f)
#define ENEMY_TIMEOUT_REWARD (-1.0f)
#define ENEMY_PROGRESS_REWARD 0.5f
#define ENEMY_HEALTH_LOSS_REWARD 0.01f
#define ENEMY_NEAR_MISS_REWARD 0.1f
#define ENEMY_NEAR_MISS_MARGIN (2.0f * MISSILE_RADIUS)

typedef struct {
    Mlp actor;
    Mlp critic;
    Adam actor_opt;
    Adam critic_opt;
} EnemyModel;

typedef struct {
    int steps;
    int envs;
    float *obs;
    int *actions;
    float *old_logp;
    float *values;
    float *rewards;
    unsigned char *dones;
    float *advantages;
    float *returns;
} EnemyRollout;

typedef struct {
    Env *envs;
    uint32_t *target_ids;
    Pcg32 *enemy_rngs;
    Pcg32 *ship_rngs;
    Pcg32 seed_rng;
    int n;
    int episode_ticks;
    Policy ship_policy;
} ArenaBatch;

typedef struct {
    EnemyModel model;
    ArenaBatch batch;
    uint64_t seed;
    int done;
    int target_done;
    int escaped;
    int killed;
} PlayerState;

static float clamp_unit(float x) {
    return x < -1.0f ? -1.0f : (x > 1.0f ? 1.0f : x);
}

static Enemy *find_enemy(Env *env, uint32_t id) {
    for (int k = 0; k < env->n_enemies; k++) {
        if (env->enemies[k].id == id) {
            return &env->enemies[k];
        }
    }
    return NULL;
}

static const Enemy *find_enemy_const(const Env *env, uint32_t id) {
    for (int k = 0; k < env->n_enemies; k++) {
        if (env->enemies[k].id == id) {
            return &env->enemies[k];
        }
    }
    return NULL;
}

int enemy_ppo_observe(const Env *env, uint32_t enemy_id, float obs[ENEMY_OBS_DIM]) {
    const Enemy *enemy = find_enemy_const(env, enemy_id);
    if (!enemy || !obs) {
        return -1;
    }
    memset(obs, 0, ENEMY_OBS_DIM * sizeof *obs);
    int at = 0;
    const float max_x = ARENA_SIZE - 2.0f * enemy->radius;
    obs[at++] = clamp_unit(2.0f * enemy->x / max_x - 1.0f);
    obs[at++] = clamp_unit(2.0f * enemy->y / ARENA_SIZE - 1.0f);
    obs[at++] = clamp_unit(enemy->vx / ENEMY_START_VX);
    obs[at++] = clamp_unit(enemy->vy / 1000.0f);
    obs[at++] = clamp_unit(enemy->health / ENEMY_TYPES[enemy->type].health);
    for (int t = 0; t < ENEMY_TYPE_FEATURES; t++) {
        obs[at++] = enemy->type == t ? 1.0f : 0.0f;
    }

    for (int i = 0; i < NUM_SHIPS; i++) {
        const Ship *ship = &env->ships[i];
        obs[at++] = clamp_unit(((ship->x + SHIP_RADIUS) - (enemy->x + enemy->radius)) / ARENA_SIZE);
        obs[at++] = clamp_unit(((ship->y + SHIP_RADIUS) - (enemy->y + enemy->radius)) / ARENA_SIZE);
        obs[at++] = clamp_unit(ship->health / SHIP_HEALTH);
        obs[at++] = ship->alive ? 1.0f : 0.0f;
    }

    unsigned char used[MAX_MISSILES] = {0};
    for (int slot = 0; slot < ENEMY_MISSILE_SLOTS; slot++) {
        int best = -1;
        float best_d2 = 0.0f;
        for (int m = 0; m < env->n_missiles; m++) {
            if (used[m]) {
                continue;
            }
            float dx = (env->missiles[m].x + MISSILE_RADIUS) - (enemy->x + enemy->radius);
            float dy = (env->missiles[m].y + MISSILE_RADIUS) - (enemy->y + enemy->radius);
            float d2 = dx * dx + dy * dy;
            if (best < 0 || d2 < best_d2) {
                best = m;
                best_d2 = d2;
            }
        }
        if (best >= 0) {
            const Missile *missile = &env->missiles[best];
            used[best] = 1;
            obs[at++] = clamp_unit((missile->x + MISSILE_RADIUS - enemy->x - enemy->radius) / ARENA_SIZE);
            obs[at++] = clamp_unit((missile->y + MISSILE_RADIUS - enemy->y - enemy->radius) / ARENA_SIZE);
            obs[at++] = clamp_unit(missile->vy / 1000.0f);
        } else {
            at += ENEMY_MISSILE_FEATURES;
        }
    }
    return at == ENEMY_OBS_DIM ? 0 : -1;
}

float enemy_ppo_near_miss_reward(const Enemy *enemy_before, const Missile *missiles_before, int n_missiles_before,
                                 const Enemy *enemy_after, const Missile *missiles_after, int n_missiles_after) {
    if (!enemy_before || !missiles_before || n_missiles_before < 0 || !enemy_after || !missiles_after ||
        n_missiles_after < 0 || enemy_after->dead || enemy_after->health < enemy_before->health) {
        return 0.0f;
    }

    float reward = 0.0f;
    const float hit_distance = MISSILE_RADIUS + enemy_before->radius;
    for (int i = 0; i < n_missiles_before; i++) {
        const Missile *before = &missiles_before[i];
        const Missile *after = NULL;
        for (int j = 0; j < n_missiles_after; j++) {
            if (missiles_after[j].id == before->id) {
                after = &missiles_after[j];
                break;
            }
        }
        if (!after) {
            continue;
        }

        float dy_before = before->y + MISSILE_RADIUS - enemy_before->y - enemy_before->radius;
        float dy_after = after->y + MISSILE_RADIUS - enemy_after->y - enemy_after->radius;
        if (dy_before <= 0.0f || dy_after > 0.0f) {
            continue;
        }

        float crossing = dy_before / (dy_before - dy_after);
        float dx_before = before->x + MISSILE_RADIUS - enemy_before->x - enemy_before->radius;
        float dx_after = after->x + MISSILE_RADIUS - enemy_after->x - enemy_after->radius;
        float miss_distance = fabsf(dx_before + crossing * (dx_after - dx_before));
        float clearance = miss_distance - hit_distance;
        if (clearance >= 0.0f && clearance < ENEMY_NEAR_MISS_MARGIN) {
            reward += ENEMY_NEAR_MISS_REWARD * (1.0f - clearance / ENEMY_NEAR_MISS_MARGIN);
        }
    }
    return reward;
}

static float categorical_prob(const float logits[ENEMY_ACTIONS], float p[ENEMY_ACTIONS]) {
    float max = logits[0];
    for (int i = 1; i < ENEMY_ACTIONS; i++) {
        if (logits[i] > max) {
            max = logits[i];
        }
    }
    float total = 0.0f;
    for (int i = 0; i < ENEMY_ACTIONS; i++) {
        p[i] = expf(logits[i] - max);
        total += p[i];
    }
    for (int i = 0; i < ENEMY_ACTIONS; i++) {
        p[i] /= total;
    }
    return max + logf(total);
}

static int sample_action(const float logits[ENEMY_ACTIONS], Pcg32 *rng, float *logp) {
    float p[ENEMY_ACTIONS];
    float logsum = categorical_prob(logits, p);
    float draw = pcg32_float(rng);
    float cdf = 0.0f;
    int action = ENEMY_ACTIONS - 1;
    for (int i = 0; i < ENEMY_ACTIONS; i++) {
        cdf += p[i];
        if (draw < cdf) {
            action = i;
            break;
        }
    }
    *logp = logits[action] - logsum;
    return action;
}

static float action_logp(const float logits[ENEMY_ACTIONS], int action, float p[ENEMY_ACTIONS]) {
    float logsum = categorical_prob(logits, p);
    return logits[action] - logsum;
}

static int model_init(EnemyModel *model, int hidden, Pcg32 *rng) {
    memset(model, 0, sizeof *model);
    if (mlp_init(&model->actor, ENEMY_OBS_DIM, hidden, ENEMY_ACTIONS, 0.01f, rng) != 0 ||
        mlp_init(&model->critic, ENEMY_OBS_DIM, hidden, 1, 1.0f, rng) != 0 ||
        adam_init(&model->actor_opt, model->actor.n_params) != 0 ||
        adam_init(&model->critic_opt, model->critic.n_params) != 0) {
        mlp_free(&model->actor);
        mlp_free(&model->critic);
        adam_free(&model->actor_opt);
        adam_free(&model->critic_opt);
        return -1;
    }
    return 0;
}

static void model_free(EnemyModel *model) {
    mlp_free(&model->actor);
    mlp_free(&model->critic);
    adam_free(&model->actor_opt);
    adam_free(&model->critic_opt);
    memset(model, 0, sizeof *model);
}

static int model_load(EnemyModel *model, const char *path, Pcg32 *rng) {
    int n_nets, sizes[4];
    int rc = checkpoint_shape(path, &n_nets, sizes, NULL);
    if (rc != 0) {
        return rc;
    }
    if (n_nets != 2 || sizes[0] != ENEMY_OBS_DIM || sizes[3] != ENEMY_ACTIONS || sizes[1] <= 0 ||
        sizes[1] > 4096) {
        return CKPT_SHAPE;
    }
    if (model_init(model, sizes[1], rng) != 0) {
        return CKPT_IO;
    }
    if (model->critic.sizes[0] != ENEMY_OBS_DIM || model->critic.sizes[1] != sizes[1] ||
        model->critic.sizes[2] != sizes[2] || sizes[2] != sizes[1] || sizes[3] != ENEMY_ACTIONS) {
        model_free(model);
        return CKPT_SHAPE;
    }
    Mlp *nets[2] = {&model->actor, &model->critic};
    rc = checkpoint_load(path, nets, 2, NULL);
    if (rc != 0) {
        model_free(model);
    }
    return rc;
}

static int model_save(EnemyModel *model, const char *path, uint32_t update) {
    Mlp *nets[2] = {&model->actor, &model->critic};
    return checkpoint_save(path, nets, 2, update);
}

static int enemy_rollout_init(EnemyRollout *r, int steps, int envs) {
    memset(r, 0, sizeof *r);
    r->steps = steps;
    r->envs = envs;
    size_t n = (size_t)steps * envs;
    r->obs = calloc(n * ENEMY_OBS_DIM, sizeof *r->obs);
    r->actions = calloc(n, sizeof *r->actions);
    r->old_logp = calloc(n, sizeof *r->old_logp);
    r->values = calloc(n, sizeof *r->values);
    r->rewards = calloc(n, sizeof *r->rewards);
    r->dones = calloc(n, sizeof *r->dones);
    r->advantages = calloc(n, sizeof *r->advantages);
    r->returns = calloc(n, sizeof *r->returns);
    return r->obs && r->actions && r->old_logp && r->values && r->rewards && r->dones && r->advantages && r->returns
               ? 0
               : -1;
}

static void enemy_rollout_free(EnemyRollout *r) {
    free(r->obs);
    free(r->actions);
    free(r->old_logp);
    free(r->values);
    free(r->rewards);
    free(r->dones);
    free(r->advantages);
    free(r->returns);
    memset(r, 0, sizeof *r);
}

static int enemy_rollout_index(const EnemyRollout *r, int t, int n) {
    return t * r->envs + n;
}

static void compute_gae(EnemyRollout *r, const float *last_values, float gamma, float lambda) {
    for (int n = 0; n < r->envs; n++) {
        float carry = 0.0f;
        for (int t = r->steps - 1; t >= 0; t--) {
            int i = enemy_rollout_index(r, t, n);
            float next;
            if (r->dones[i]) {
                next = 0.0f;
                carry = 0.0f;
            } else {
                next = t == r->steps - 1 ? last_values[n] : r->values[enemy_rollout_index(r, t + 1, n)];
            }
            float delta = r->rewards[i] + gamma * next - r->values[i];
            carry = delta + gamma * lambda * carry;
            r->advantages[i] = carry;
            r->returns[i] = carry + r->values[i];
        }
    }
}

static void shuffle_indices(int *order, int count, Pcg32 *rng) {
    for (int i = count - 1; i > 0; i--) {
        int j = (int)pcg32_bounded(rng, (uint32_t)(i + 1));
        int tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
    }
}

static void enemy_ppo_update(EnemyModel *model, EnemyRollout *r, const PpoConfig *cfg, Pcg32 *rng) {
    int count = r->steps * r->envs;
    double mean = 0.0, sq = 0.0;
    for (int i = 0; i < count; i++) {
        mean += r->advantages[i];
    }
    mean /= count;
    for (int i = 0; i < count; i++) {
        double d = r->advantages[i] - mean;
        sq += d * d;
    }
    float inv_std = (float)(1.0 / sqrt(sq / count + 1e-8));
    for (int i = 0; i < count; i++) {
        r->advantages[i] = (r->advantages[i] - (float)mean) * inv_std;
    }

    int minibatches = cfg->minibatches < count ? cfg->minibatches : count;
    int batch_cap = (count + minibatches - 1) / minibatches;
    float *obs = calloc((size_t)batch_cap * ENEMY_OBS_DIM, sizeof *obs);
    float *g_logits = calloc((size_t)batch_cap * ENEMY_ACTIONS, sizeof *g_logits);
    float *g_values = calloc((size_t)batch_cap, sizeof *g_values);
    float *values = calloc((size_t)batch_cap, sizeof *values);
    int *actions = calloc((size_t)batch_cap, sizeof *actions);
    float *old_logp = calloc((size_t)batch_cap, sizeof *old_logp);
    float *adv = calloc((size_t)batch_cap, sizeof *adv);
    float *ret = calloc((size_t)batch_cap, sizeof *ret);
    int *order = calloc((size_t)count, sizeof *order);
    if (!obs || !g_logits || !g_values || !values || !actions || !old_logp || !adv || !ret || !order) {
        fprintf(stderr, "enemy PPO: out of memory during update\n");
        abort();
    }
    for (int i = 0; i < count; i++) {
        order[i] = i;
    }
    float *groups[2] = {model->actor.grads, model->critic.grads};
    int sizes[2] = {model->actor.n_params, model->critic.n_params};
    for (int epoch = 0; epoch < cfg->epochs; epoch++) {
        shuffle_indices(order, count, rng);
        for (int mb = 0; mb < minibatches; mb++) {
            int lo = count * mb / minibatches;
            int hi = count * (mb + 1) / minibatches;
            int b = hi - lo;
            for (int j = 0; j < b; j++) {
                int i = order[lo + j];
                memcpy(obs + (size_t)j * ENEMY_OBS_DIM, r->obs + (size_t)i * ENEMY_OBS_DIM,
                       ENEMY_OBS_DIM * sizeof *obs);
                actions[j] = r->actions[i];
                old_logp[j] = r->old_logp[i];
                adv[j] = r->advantages[i];
                ret[j] = r->returns[i];
            }
            const float *logits = mlp_forward(&model->actor, obs, b);
            memcpy(values, mlp_forward(&model->critic, obs, b), (size_t)b * sizeof *values);
            memset(g_logits, 0, (size_t)b * ENEMY_ACTIONS * sizeof *g_logits);
            memset(g_values, 0, (size_t)b * sizeof *g_values);
            float inv_b = 1.0f / b;
            for (int j = 0; j < b; j++) {
                const float *z = logits + (size_t)j * ENEMY_ACTIONS;
                float p[ENEMY_ACTIONS];
                float lp = action_logp(z, actions[j], p);
                float ratio = expf(lp - old_logp[j]);
                int active_pg = !((adv[j] >= 0.0f && ratio > 1.0f + cfg->clip) ||
                                  (adv[j] < 0.0f && ratio < 1.0f - cfg->clip));
                float entropy = 0.0f;
                for (int a = 0; a < ENEMY_ACTIONS; a++) {
                    if (p[a] > 0.0f) {
                        entropy -= p[a] * logf(p[a]);
                    }
                }
                for (int a = 0; a < ENEMY_ACTIONS; a++) {
                    float grad = active_pg ? -adv[j] * ratio * ((a == actions[j] ? 1.0f : 0.0f) - p[a]) : 0.0f;
                    if (p[a] > 0.0f) {
                        grad += cfg->ent_coef * p[a] * (logf(p[a]) + entropy);
                    }
                    g_logits[(size_t)j * ENEMY_ACTIONS + a] = grad * inv_b;
                }
                g_values[j] = cfg->vf_coef * (values[j] - ret[j]) * inv_b;
            }
            mlp_zero_grad(&model->actor);
            mlp_zero_grad(&model->critic);
            mlp_backward(&model->actor, obs, g_logits, b);
            mlp_backward(&model->critic, obs, g_values, b);
            clip_grad_norm(groups, sizes, 2, cfg->max_grad_norm);
            adam_step(&model->actor_opt, model->actor.params, model->actor.grads, cfg->lr);
            adam_step(&model->critic_opt, model->critic.params, model->critic.grads, cfg->lr);
        }
    }
    free(obs);
    free(g_logits);
    free(g_values);
    free(values);
    free(actions);
    free(old_logp);
    free(adv);
    free(ret);
    free(order);
}

static uint64_t next_seed(Pcg32 *rng) {
    uint64_t seed = ((uint64_t)pcg32_next(rng) << 32) | pcg32_next(rng);
    return seed < 100000u ? seed + 100000u : seed;
}

int enemy_ppo_arena_init(Env *env, uint64_t seed, int episode_ticks, uint32_t *target_id) {
    if (!env || !target_id || episode_ticks <= 0) {
        return -1;
    }
    env_reset(env, seed);
    env->max_ticks = episode_ticks;
    env->spawn_timer = ENEMY_SPAWN_TICKS;
    int type = (int)pcg32_bounded(&env->rng, 3);
    float x = (float)pcg32_bounded(&env->rng, (uint32_t)ENEMY_SPAWN_MAX_X + 1);
    Enemy *enemy = env_add_enemy(env, type, x, -ENEMY_RADIUS);
    if (!enemy) {
        return -1;
    }
    *target_id = enemy->id;
    return 0;
}

static int arena_batch_init(ArenaBatch *batch, int n, int episode_ticks, uint64_t seed, const char *ship_checkpoint) {
    memset(batch, 0, sizeof *batch);
    batch->n = n;
    batch->episode_ticks = episode_ticks;
    batch->envs = calloc((size_t)n, sizeof *batch->envs);
    batch->target_ids = calloc((size_t)n, sizeof *batch->target_ids);
    batch->enemy_rngs = calloc((size_t)n, sizeof *batch->enemy_rngs);
    batch->ship_rngs = calloc((size_t)n, sizeof *batch->ship_rngs);
    if (!batch->envs || !batch->target_ids || !batch->enemy_rngs || !batch->ship_rngs) {
        return -1;
    }
    pcg32_seed(&batch->seed_rng, seed, 0xeeeeu);
    int rc = policy_ppo(&batch->ship_policy, ship_checkpoint, 1);
    if (rc != 0) {
        fprintf(stderr, "enemy PPO: cannot load ship checkpoint: %s\n", checkpoint_error(rc));
        return -1;
    }
    for (int nenv = 0; nenv < n; nenv++) {
        pcg32_seed(&batch->enemy_rngs[nenv], seed, 0xe11u + (uint64_t)nenv);
        pcg32_seed(&batch->ship_rngs[nenv], seed, 0x51u + (uint64_t)nenv);
        if (enemy_ppo_arena_init(&batch->envs[nenv], next_seed(&batch->seed_rng), episode_ticks,
                                 &batch->target_ids[nenv]) != 0) {
            return -1;
        }
    }
    return 0;
}

static void arena_batch_free(ArenaBatch *batch) {
    policy_free(&batch->ship_policy);
    free(batch->envs);
    free(batch->target_ids);
    free(batch->enemy_rngs);
    free(batch->ship_rngs);
    memset(batch, 0, sizeof *batch);
}

static void arena_batch_free_arrays(ArenaBatch *batch) {
    free(batch->envs);
    free(batch->target_ids);
    free(batch->enemy_rngs);
    free(batch->ship_rngs);
    memset(batch, 0, sizeof *batch);
}

static void arena_reset(ArenaBatch *batch, int n) {
    if (enemy_ppo_arena_init(&batch->envs[n], next_seed(&batch->seed_rng), batch->episode_ticks,
                             &batch->target_ids[n]) != 0) {
        fprintf(stderr, "enemy PPO: could not add controlled enemy\n");
        abort();
    }
}

static int choose_enemy_action(EnemyModel *model, const Env *env, uint32_t enemy_id, Pcg32 *rng, int random_policy,
                               int stochastic) {
    if (random_policy) {
        return (int)pcg32_bounded(rng, ENEMY_ACTIONS);
    }
    float obs[ENEMY_OBS_DIM];
    if (enemy_ppo_observe(env, enemy_id, obs) != 0) {
        fprintf(stderr, "enemy PPO: could not observe enemy %u\n", enemy_id);
        abort();
    }
    const float *logits = mlp_forward(&model->actor, obs, 1);
    if (stochastic) {
        float logp;
        return sample_action(logits, rng, &logp);
    }
    int action = 0;
    for (int a = 1; a < ENEMY_ACTIONS; a++) {
        if (logits[a] > logits[action]) {
            action = a;
        }
    }
    return action;
}

static void set_enemy_action(Enemy *enemy, int action) {
    enemy->vx = (float)(action - 1) * ENEMY_START_VX;
    enemy->zigzag_distance = FLT_MAX;
    enemy->zigzag_traveled = 0.0f;
}

static void apply_policy_to_enemies(EnemyModel *model, Env *env, Pcg32 *rng, int random_policy, int stochastic,
                                    uint32_t skip_id) {
    for (int k = 0; k < env->n_enemies; k++) {
        Enemy *enemy = &env->enemies[k];
        if (enemy->dead || enemy->id == skip_id) {
            continue;
        }
        int action = choose_enemy_action(model, env, enemy->id, rng, random_policy, stochastic);
        set_enemy_action(enemy, action);
    }
}

typedef struct {
    int done;
    int escaped;
    int killed;
    float reward;
} StepResult;

static StepResult step_arena(EnemyModel *model, ArenaBatch *batch, int n, int action, int random_policy,
                             int other_stochastic) {
    Env *env = &batch->envs[n];
    Enemy *enemy = find_enemy(env, batch->target_ids[n]);
    if (!enemy || enemy->dead) {
        return (StepResult){1, 0, 1, ENEMY_DEATH_REWARD};
    }
    float old_y = enemy->y;
    float old_health = enemy->health;
    Enemy enemy_before = *enemy;
    Missile missiles_before[MAX_MISSILES];
    int n_missiles_before = env->n_missiles;
    memcpy(missiles_before, env->missiles, (size_t)n_missiles_before * sizeof *missiles_before);
    int escapes_before = env->stats.escapes;
    set_enemy_action(enemy, action);
    apply_policy_to_enemies(model, env, &batch->enemy_rngs[n], random_policy, other_stochastic, enemy->id);
    ShipAction ships[NUM_SHIPS];
    policy_act(&batch->ship_policy, env, &batch->ship_rngs[n], ships);
    env_step(env, ships);

    enemy = find_enemy(env, batch->target_ids[n]);
    if (!enemy) {
        int escaped = env->stats.escapes > escapes_before;
        return (StepResult){1, escaped, !escaped, escaped ? ENEMY_EPISODE_REWARD : ENEMY_DEATH_REWARD};
    }
    float progress = fmaxf(0.0f, enemy->y - old_y) / ARENA_SIZE;
    float health_loss = fmaxf(0.0f, old_health - enemy->health);
    float reward = ENEMY_PROGRESS_REWARD * progress - ENEMY_HEALTH_LOSS_REWARD * health_loss +
                   enemy_ppo_near_miss_reward(&enemy_before, missiles_before, n_missiles_before, enemy, env->missiles,
                                              env->n_missiles);
    if (enemy->dead) {
        return (StepResult){1, 0, 1, reward + ENEMY_DEATH_REWARD};
    }
    if (env->terminated || env->truncated) {
        return (StepResult){1, 0, 0, reward + ENEMY_TIMEOUT_REWARD};
    }
    return (StepResult){0, 0, 0, reward};
}

static int mkdir_p(const char *path) {
    char copy[4096];
    if (!path || snprintf(copy, sizeof copy, "%s", path) >= (int)sizeof copy) {
        return -1;
    }
    for (char *p = copy + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(copy, 0755) != 0 && errno != EEXIST) {
                return -1;
            }
            *p = '/';
        }
    }
    return mkdir(copy, 0755) == 0 || errno == EEXIST ? 0 : -1;
}

static int save_model(EnemyModel *model, const char *run_dir, int update) {
    char path[4096];
    if (snprintf(path, sizeof path, "%s/final.bin", run_dir) >= (int)sizeof path) {
        return -1;
    }
    int rc = model_save(model, path, (uint32_t)update);
    if (rc != 0) {
        fprintf(stderr, "enemy PPO: could not save %s: %s\n", path, checkpoint_error(rc));
        return -1;
    }
    return 0;
}

static void collect_rollout(EnemyModel *model, ArenaBatch *batch, EnemyRollout *rollout, PpoConfig *ppo,
                            Pcg32 *update_rng, long long *episodes, long long *escapes, double *reward_sum,
                            double *elapsed_sum) {
    int envs = batch->n;
    float *last_values = calloc((size_t)envs, sizeof *last_values);
    if (!last_values) {
        abort();
    }
    for (int t = 0; t < rollout->steps; t++) {
        for (int n = 0; n < envs; n++) {
            int i = enemy_rollout_index(rollout, t, n);
            float *obs = rollout->obs + (size_t)i * ENEMY_OBS_DIM;
            if (enemy_ppo_observe(&batch->envs[n], batch->target_ids[n], obs) != 0) {
                fprintf(stderr, "enemy PPO: controlled enemy missing before action\n");
                abort();
            }
            const float *logits = mlp_forward(&model->actor, obs, 1);
            float logp;
            rollout->actions[i] = sample_action(logits, &batch->enemy_rngs[n], &logp);
            rollout->old_logp[i] = logp;
            rollout->values[i] = mlp_forward(&model->critic, obs, 1)[0];
            StepResult result = step_arena(model, batch, n, rollout->actions[i], 0, 1);
            rollout->rewards[i] = result.reward;
            rollout->dones[i] = (unsigned char)result.done;
            *reward_sum += result.reward;
            if (result.done) {
                (*episodes)++;
                *escapes += result.escaped;
                *elapsed_sum += (double)batch->envs[n].tick / TICK_HZ;
                arena_reset(batch, n);
            }
        }
    }
    for (int n = 0; n < envs; n++) {
        float obs[ENEMY_OBS_DIM];
        if (enemy_ppo_observe(&batch->envs[n], batch->target_ids[n], obs) != 0) {
            abort();
        }
        last_values[n] = mlp_forward(&model->critic, obs, 1)[0];
    }
    compute_gae(rollout, last_values, ppo->gamma, ppo->lambda);
    enemy_ppo_update(model, rollout, ppo, update_rng);
    free(last_values);
}

int enemy_ppo_train(const EnemyTrainConfig *cfg) {
    if (!cfg || !cfg->run_dir || !cfg->ship_checkpoint || cfg->updates <= 0 || cfg->envs <= 0 || cfg->steps <= 0 ||
        cfg->hidden <= 0 || cfg->hidden > 4096 || mkdir_p(cfg->run_dir) != 0) {
        fprintf(stderr, "enemy PPO: invalid configuration or cannot create run directory\n");
        return -1;
    }
    char metrics_path[4096];
    if (snprintf(metrics_path, sizeof metrics_path, "%s/metrics.csv", cfg->run_dir) >= (int)sizeof metrics_path) {
        return -1;
    }
    FILE *metrics = fopen(metrics_path, "w");
    if (!metrics) {
        fprintf(stderr, "enemy PPO: cannot write %s\n", metrics_path);
        return -1;
    }
    fputs("update,decisions,episodes,escape_rate,mean_reward,mean_episode_seconds\n", metrics);

    Pcg32 init_rng, update_rng;
    pcg32_seed(&init_rng, cfg->seed, 0xe111u);
    pcg32_seed(&update_rng, cfg->seed, 0xe112u);
    EnemyModel model = {0};
    EnemyRollout rollout = {0};
    ArenaBatch batch = {0};
    int model_ok = model_init(&model, cfg->hidden, &init_rng) == 0;
    int rollout_ok = model_ok && enemy_rollout_init(&rollout, cfg->steps, cfg->envs) == 0;
    int batch_ok = rollout_ok && arena_batch_init(&batch, cfg->envs, EPISODE_TICKS, cfg->seed, cfg->ship_checkpoint) == 0;
    if (!batch_ok) {
        enemy_rollout_free(&rollout);
        if (model_ok) {
            model_free(&model);
        }
        arena_batch_free(&batch);
        fclose(metrics);
        return -1;
    }

    PpoConfig ppo = ppo_config_default();
    ppo.minibatches = ppo.minibatches < cfg->steps * cfg->envs ? ppo.minibatches : cfg->steps * cfg->envs;
    long long episodes = 0, escapes = 0;
    double reward_sum = 0.0, elapsed_sum = 0.0;
    long long decisions = 0;
    for (int update = 1; update <= cfg->updates; update++) {
        collect_rollout(&model, &batch, &rollout, &ppo, &update_rng, &episodes, &escapes, &reward_sum, &elapsed_sum);
        decisions += (long long)cfg->envs * cfg->steps;
        fprintf(metrics, "%d,%lld,%lld,%.6f,%.6f,%.3f\n", update, decisions, episodes,
                episodes ? (double)escapes / episodes : 0.0, decisions ? reward_sum / decisions : 0.0,
                episodes ? elapsed_sum / episodes : 0.0);
        if (fflush(metrics) != 0) {
            fprintf(stderr, "enemy PPO: failed writing %s\n", metrics_path);
            break;
        }
    }
    int rc = save_model(&model, cfg->run_dir, cfg->updates);
    if (fclose(metrics) != 0) {
        rc = -1;
    }
    printf("enemy PPO: %d updates, %lld decisions, %lld episodes; escape rate %.3f -> %s/final.bin\n", cfg->updates,
           decisions, episodes, episodes ? (double)escapes / episodes : 0.0, cfg->run_dir);
    enemy_rollout_free(&rollout);
    arena_batch_free_arrays(&batch);
    model_free(&model);
    return rc;
}

typedef struct {
    EnemyModel model;
} ShipTrainingOpponent;

static int reset_ships_training_arena(Env *env, uint64_t seed, int episode_ticks, void *context) {
    uint32_t target_id;
    (void)context;
    return enemy_ppo_arena_init(env, seed, episode_ticks, &target_id);
}

static int step_ships_training_arena(Env *env, const ShipAction actions[NUM_SHIPS], void *context) {
    ShipTrainingOpponent *opponent = context;
    apply_policy_to_enemies(&opponent->model, env, NULL, 0, 0, 0);
    env_step(env, actions);
    return 0;
}

int enemy_ppo_train_ships(const RunConfig *cfg, const char *enemy_checkpoint) {
    if (!cfg || !enemy_checkpoint) {
        fprintf(stderr, "ship training: an enemy checkpoint is required\n");
        return -1;
    }
    ShipTrainingOpponent opponent = {0};
    Pcg32 rng;
    pcg32_seed(&rng, cfg->train.seed, 0xe113u);
    int rc = model_load(&opponent.model, enemy_checkpoint, &rng);
    if (rc != 0) {
        fprintf(stderr, "ship training: cannot load enemy checkpoint %s: %s\n", enemy_checkpoint,
                checkpoint_error(rc));
        return -1;
    }
    TrainerEnvOps ops = {
        .reset = reset_ships_training_arena,
        .step = step_ships_training_arena,
        .context = &opponent,
    };
    int result = train_run_with_ops(cfg, &ops);
    model_free(&opponent.model);
    return result;
}

static int evaluate_one(EnemyModel *enemy_model, int random_policy, Policy *ship_policy, uint64_t seed,
                        EnemyEpisode *result) {
    ArenaBatch batch;
    memset(&batch, 0, sizeof batch);
    batch.n = 1;
    batch.episode_ticks = EPISODE_TICKS;
    batch.envs = calloc(1, sizeof *batch.envs);
    batch.target_ids = calloc(1, sizeof *batch.target_ids);
    batch.enemy_rngs = calloc(1, sizeof *batch.enemy_rngs);
    batch.ship_rngs = calloc(1, sizeof *batch.ship_rngs);
    if (!batch.envs || !batch.target_ids || !batch.enemy_rngs || !batch.ship_rngs) {
        arena_batch_free_arrays(&batch);
        return -1;
    }
    batch.ship_policy = *ship_policy;
    pcg32_seed(&batch.seed_rng, seed, 0xeeeeu);
    pcg32_seed(batch.enemy_rngs, seed, 0xe11u);
    pcg32_seed(batch.ship_rngs, seed, 0x51u);
    if (enemy_ppo_arena_init(batch.envs, seed, EPISODE_TICKS, batch.target_ids) != 0) {
        free(batch.envs);
        free(batch.target_ids);
        free(batch.enemy_rngs);
        free(batch.ship_rngs);
        return -1;
    }
    Env *env = &batch.envs[0];
    uint32_t target_id = batch.target_ids[0];
    double total_reward = 0.0;
    int escaped = 0, killed = 0, target_done = 0;
    while (!env->terminated && !env->truncated) {
        if (!target_done) {
            Enemy *target = find_enemy(env, target_id);
            if (!target || target->dead) {
                target_done = 1;
                killed = target && target->dead;
            } else {
                int action = choose_enemy_action(enemy_model, env, target_id, batch.enemy_rngs, random_policy, 0);
                StepResult step = step_arena(enemy_model, &batch, 0, action, random_policy, 0);
                total_reward += step.reward;
                if (step.done) {
                    target_done = 1;
                    escaped = step.escaped;
                    killed = step.killed;
                }
                continue;
            }
        }
        apply_policy_to_enemies(enemy_model, env, batch.enemy_rngs, random_policy, 0, 0);
        ShipAction ships[NUM_SHIPS];
        policy_act(&batch.ship_policy, env, batch.ship_rngs, ships);
        env_step(env, ships);
    }

    int ammo_remaining = 0;
    int all_ships_out_of_ammo = 1;
    int last_ammo_out_tick = 0;
    double ship_health_remaining = 0.0, explosion_damage = 0.0, damage_taken = 0.0;
    int ships_lost = 0;
    for (int i = 0; i < NUM_SHIPS; i++) {
        const Ship *ship = &env->ships[i];
        ammo_remaining += ship->ammo;
        ship_health_remaining += ship->health;
        explosion_damage += env->stats.bomb_damage[i];
        damage_taken += env->stats.damage_taken[i];
        ships_lost += env->stats.died[i];
        if (ship->ammo > 0) {
            all_ships_out_of_ammo = 0;
        }
        if (env->stats.ammo_out_tick[i] > last_ammo_out_tick) {
            last_ammo_out_tick = env->stats.ammo_out_tick[i];
        }
    }
    result->escaped = escaped;
    result->killed = killed;
    result->seconds = (double)env->tick / TICK_HZ;
    result->reward = total_reward;
    result->enemies_escaped = env->stats.escapes;
    result->enemies_spawned = env->stats.spawned;
    result->hits = env->stats.hits[0] + env->stats.hits[1] + env->stats.hits[2];
    result->shots = env->stats.shots[0] + env->stats.shots[1] + env->stats.shots[2];
    result->ammo_remaining = ammo_remaining;
    result->all_ships_out_of_ammo = all_ships_out_of_ammo;
    result->all_ships_ammo_out_seconds = all_ships_out_of_ammo ? (double)last_ammo_out_tick / TICK_HZ : 0.0;
    result->ship_health_remaining = ship_health_remaining;
    result->explosion_damage = explosion_damage;
    result->damage_taken = damage_taken;
    result->ships_lost = ships_lost;
    arena_batch_free_arrays(&batch);
    return 0;
}

int enemy_ppo_player_init(EnemyPpoPlayer *player, const char *enemy_checkpoint, const char *ship_checkpoint,
                         uint64_t seed) {
    if (!player || !enemy_checkpoint || !ship_checkpoint) {
        fprintf(stderr, "enemy PPO viewer: missing checkpoint or player\n");
        return -1;
    }
    player->state = NULL;
    PlayerState *state = calloc(1, sizeof *state);
    if (!state) {
        fprintf(stderr, "enemy PPO viewer: out of memory\n");
        return -1;
    }
    Pcg32 rng;
    pcg32_seed(&rng, seed, 0xe113u);
    int rc = model_load(&state->model, enemy_checkpoint, &rng);
    if (rc != 0) {
        fprintf(stderr, "enemy PPO viewer: cannot load enemy checkpoint: %s\n", checkpoint_error(rc));
        free(state);
        return -1;
    }
    if (arena_batch_init(&state->batch, 1, ENEMY_VIEW_EPISODE_SECONDS * TICK_HZ, seed, ship_checkpoint) != 0) {
        arena_batch_free(&state->batch);
        model_free(&state->model);
        free(state);
        return -1;
    }
    state->seed = seed;
    player->state = state;
    return 0;
}

void enemy_ppo_player_free(EnemyPpoPlayer *player) {
    if (!player || !player->state) {
        return;
    }
    PlayerState *state = player->state;
    arena_batch_free(&state->batch);
    model_free(&state->model);
    free(state);
    player->state = NULL;
}

const Env *enemy_ppo_player_env(const EnemyPpoPlayer *player) {
    if (!player || !player->state) {
        return NULL;
    }
    const PlayerState *state = player->state;
    return &state->batch.envs[0];
}

int enemy_ppo_player_step(EnemyPpoPlayer *player) {
    if (!player || !player->state) {
        fprintf(stderr, "enemy PPO viewer: player is not initialized\n");
        return -1;
    }
    PlayerState *state = player->state;
    if (state->done) {
        return 1;
    }
    Env *env = &state->batch.envs[0];
    if (!state->target_done) {
        float obs[ENEMY_OBS_DIM];
        if (enemy_ppo_observe(env, state->batch.target_ids[0], obs) != 0) {
            fprintf(stderr, "enemy PPO viewer: controlled enemy is missing\n");
            return -1;
        }
        int action = choose_enemy_action(&state->model, env, state->batch.target_ids[0],
                                         state->batch.enemy_rngs, 0, 0);
        StepResult result = step_arena(&state->model, &state->batch, 0, action, 0, 0);
        if (result.done) {
            state->target_done = 1;
            state->escaped = result.escaped;
            state->killed = result.killed;
        }
    } else {
        apply_policy_to_enemies(&state->model, env, state->batch.enemy_rngs, 0, 0, 0);
        ShipAction ships[NUM_SHIPS];
        policy_act(&state->batch.ship_policy, env, &state->batch.ship_rngs[0], ships);
        env_step(env, ships);
    }
    state->done = env->terminated || env->truncated;
    return state->done;
}

int enemy_ppo_player_done(const EnemyPpoPlayer *player, int *escaped, int *killed) {
    if (!player || !player->state) {
        return -1;
    }
    const PlayerState *state = player->state;
    if (escaped) {
        *escaped = state->escaped;
    }
    if (killed) {
        *killed = state->killed;
    }
    return state->done;
}

void enemy_ppo_player_reset(EnemyPpoPlayer *player) {
    if (!player || !player->state) {
        return;
    }
    PlayerState *state = player->state;
    arena_reset(&state->batch, 0);
    state->seed++;
    state->done = 0;
    state->target_done = 0;
    state->escaped = 0;
    state->killed = 0;
}

int enemy_ppo_evaluate(const char *enemy_checkpoint, const char *ship_checkpoint, int episodes, uint64_t seed,
                       const char *output, int random_policy) {
    if ((!random_policy && !enemy_checkpoint) || !ship_checkpoint || episodes <= 0 || episodes > 10000) {
        fprintf(stderr, "enemy PPO: invalid evaluation arguments\n");
        return -1;
    }
    Pcg32 rng;
    pcg32_seed(&rng, seed, 0xe113u);
    EnemyModel model = {0};
    if (!random_policy) {
        int rc = model_load(&model, enemy_checkpoint, &rng);
        if (rc != 0) {
            fprintf(stderr, "enemy PPO: cannot load enemy checkpoint: %s\n", checkpoint_error(rc));
            return -1;
        }
    }
    Policy ship_policy;
    int rc = policy_ppo(&ship_policy, ship_checkpoint, 1);
    if (rc != 0) {
        model_free(&model);
        fprintf(stderr, "enemy PPO: cannot load ship checkpoint: %s\n", checkpoint_error(rc));
        return -1;
    }
    double escaped = 0.0, killed = 0.0, seconds = 0.0, reward = 0.0;
    double enemies_escaped = 0.0, enemies_spawned = 0.0, hits = 0.0, shots = 0.0, ammo_remaining = 0.0;
    double all_ships_out = 0.0, all_ships_ammo_out_seconds = 0.0, ship_health_remaining = 0.0;
    double episode_accuracy = 0.0, damage_taken = 0.0, ships_lost = 0.0;
    for (int i = 0; i < episodes; i++) {
        EnemyEpisode result;
        if (evaluate_one(&model, random_policy, &ship_policy, seed + (uint64_t)i, &result) != 0) {
            policy_free(&ship_policy);
            model_free(&model);
            return -1;
        }
        escaped += result.escaped;
        killed += result.killed;
        seconds += result.seconds;
        reward += result.reward;
        enemies_escaped += result.enemies_escaped;
        enemies_spawned += result.enemies_spawned;
        hits += result.hits;
        shots += result.shots;
        ammo_remaining += result.ammo_remaining;
        all_ships_out += result.all_ships_out_of_ammo;
        all_ships_ammo_out_seconds += result.all_ships_ammo_out_seconds;
        ship_health_remaining += result.ship_health_remaining;
        episode_accuracy += result.shots > 0 ? (double)result.hits / result.shots : 0.0;
        damage_taken += result.damage_taken;
        ships_lost += result.ships_lost;
    }
    policy_free(&ship_policy);
    model_free(&model);
    double escape_rate = escaped / episodes;
    printf("enemy PPO on %d full episodes: target escape rate %.3f, target kill rate %.3f, "
           "mean duration %.1f s, enemies escaped/spawned %.2f/%.2f per episode, ship accuracy %.3f (%g/%g), "
           "mean combined ammo left %.2f, all ships out of ammo %g/%d episodes",
           episodes, escape_rate, killed / episodes, seconds / episodes, enemies_escaped / episodes,
           enemies_spawned / episodes,
           shots > 0.0 ? hits / shots : 0.0, hits, shots, ammo_remaining / episodes, all_ships_out, episodes);
    if (all_ships_out > 0.0) {
        printf(", mean time all ships out %.1f s", all_ships_ammo_out_seconds / all_ships_out);
    }
    printf(", mean episode accuracy %.3f, mean total ship damage taken %.2f, mean ships lost %.3f"
           ", mean combined ship health left %.2f, reward %.3f\n",
           episode_accuracy / episodes, damage_taken / episodes, ships_lost / episodes,
           ship_health_remaining / episodes, reward / episodes);
    if (output) {
        FILE *f = fopen(output, "w");
        if (!f) {
            fprintf(stderr, "enemy PPO: cannot write %s\n", output);
            return -1;
        }
        fprintf(f, "{\n  \"policy\": \"%s\",\n  \"episodes\": %d,\n  \"seed\": %llu,\n  "
                  "\"target_escape_rate\": %.6f,\n  \"target_kill_rate\": %.6f,\n"
                  "  \"mean_episode_seconds\": %.6f,\n  \"mean_enemies_escaped\": %.6f,\n"
                  "  \"mean_enemies_spawned\": %.6f,\n"
                  "  \"total_hits\": %.0f,\n  \"total_shots\": %.0f,\n  \"accuracy\": %.6f,\n"
                  "  \"mean_episode_accuracy\": %.6f,\n  \"mean_total_ship_damage_taken\": %.6f,\n"
                  "  \"mean_ships_lost_per_episode\": %.6f,\n"
                  "  \"mean_combined_ammo_remaining\": %.6f,\n"
                  "  \"episodes_all_ships_out_of_ammo\": %.0f,\n"
                  "  \"mean_time_all_ships_out_of_ammo_seconds\": ",
                random_policy ? "random" : "ppo", episodes, (unsigned long long)seed, escape_rate, killed / episodes,
                seconds / episodes, enemies_escaped / episodes, enemies_spawned / episodes,
                hits, shots, shots > 0.0 ? hits / shots : 0.0,
                episode_accuracy / episodes, damage_taken / episodes, ships_lost / episodes,
                ammo_remaining / episodes, all_ships_out);
        if (all_ships_out > 0.0) {
            fprintf(f, "%.6f", all_ships_ammo_out_seconds / all_ships_out);
        } else {
            fputs("null", f);
        }
        fprintf(f, ",\n  \"mean_combined_ship_health_remaining\": %.6f,\n  \"mean_reward\": %.6f\n}\n",
                ship_health_remaining / episodes, reward / episodes);
        if (fclose(f) != 0) {
            return -1;
        }
    }
    return 0;
}
