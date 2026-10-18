#include <assert.h>
#include <math.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Independent fixed-step environment. Rendering never changes simulation speed. */
#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 1000
#define MAX_ENEMIES 12
#define MAX_BULLETS 32
#define MAX_EXPLOSIONS 128
#define ACTION_COUNT 9
#define STATE_COUNT (7 * 7 * 3 * 3 * 3 * 3 * 3)
#define MAX_EPISODE_STEPS 1800
#define START_HEALTH 1000
#define SIMULATION_TIME_STEP (1.0f / 30.0f)
typedef struct {
    float health;
    float damage;
    float acceleration;
    float attack_cooldown_seconds;
    int points;
} EnemyStats;
static const EnemyStats enemy_stats[3] = {
    {25, 5, 90, 1, 2}, {37.5f, 7, 72, 1.2f, 4}, {50, 10, 54, 1.4f, 8}};
#define MODEL_PATH "models/zigzag_policy.bin"
typedef struct {
    float x, y;
    bool active;
    int type, lifetime_ticks;
    float health, attack_cooldown_remaining, velocity_y, velocity_x, patrol_left, patrol_right;
} Entity;

typedef struct {
    float x, y;
    int health, tick, kills, hits, shots, landed, spawned, weapon_cooldown_ticks, score;
    float spawn_timer;
    bool done;
    Entity enemies[MAX_ENEMIES], bullets[MAX_BULLETS], blasts[MAX_EXPLOSIONS];
} GameState;

typedef struct {
    float q_values[STATE_COUNT][ACTION_COUNT];
    uint32_t episodes;
} LearningAgent;
static LearningAgent agent;
static uint32_t random_seed = 12345;
/* Xorshift generator keeps training and evaluation reproducible from a seed. */
static uint32_t random_uint(void) {
    random_seed ^= random_seed << 13;
    random_seed ^= random_seed >> 17;
    random_seed ^= random_seed << 5;
    return random_seed;
}

static float random_float(void) {
    return (random_uint() >> 8) * (1.0f / 16777216.0f);
}

static float clamp_float(float x, float lo, float hi) {
    return x < lo ? lo : x > hi ? hi : x;
}

static void reset_game(GameState *game) {
    *game = (GameState){.x = WINDOW_WIDTH / 2, .y = WINDOW_HEIGHT - 90, .health = START_HEALTH};
}

static int displacement_bucket(float displacement) {
    if (displacement < -240) {
        return 0;
    }
    if (displacement < -90) {
        return 1;
    }
    if (displacement < -25) {
        return 2;
    }
    if (displacement < 25) {
        return 3;
    }
    if (displacement < 90) {
        return 4;
    }
    if (displacement < 240) {
        return 5;
    }
    return 6;
}

/* Encode displacement, screen region, proximity, weapon cooldown, and hazard type
 * into one index in the Q-table. Each multiplication adds a categorical feature. */
static int encode_state(const GameState *game) {
    float best = 1e20f, dx = 0, dy = 0;
    bool found = false;
    int threat = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (game->enemies[i].active) {
            float x = game->enemies[i].x - game->x, y = game->enemies[i].y - game->y,
                  d = x * x + y * y;
            if (d < best) {
                best = d;
                dx = x;
                dy = y;
                found = true;
                threat = 1;
            }
        }
    }
    /* Explosions have larger collision radii: prioritize their effective distance. */
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (game->blasts[i].active) {
            float x = game->blasts[i].x - game->x, y = game->blasts[i].y - game->y;
            float surface = fmaxf(0, sqrtf(x * x + y * y) - 35), d = surface * surface;
            if (d < best) {
                best = d;
                dx = x;
                dy = y;
                found = true;
                threat = 2;
            }
        }
    }
    int bx = (int)(game->x / (WINDOW_WIDTH / 3.0f)),
        by = (int)((game->y - 60) / ((WINDOW_HEIGHT - 80) / 3.0f));
    if (bx > 2) {
        bx = 2;
    }
    if (by > 2) {
        by = 2;
    }
    int state_index = displacement_bucket(dx) * 7 + displacement_bucket(dy);
    state_index = state_index * 3 + bx;
    state_index = state_index * 3 + by;
    state_index = state_index * 3 + (found ? (best < 100 * 100 ? 1 : 2) : 0);
    state_index = state_index * 3 + (game->weapon_cooldown_ticks == 0  ? 0
                                     : game->weapon_cooldown_ticks < 5 ? 1
                                                                       : 2);
    return state_index * 3 + threat;
}

/* Destroyed enemies become moving explosions that can still damage the ship. */
static void create_explosion(GameState *game, Entity *enemy) {
    int slot = -1;
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!game->blasts[i].active) {
            slot = i;
            break;
        }
    }
    assert(slot >= 0); /* At most 80 spawns in a 1,800-step episode. */
    game->blasts[slot] = (Entity){.x = enemy->x,
                                  .y = enemy->y,
                                  .active = true,
                                  .velocity_y = 100,
                                  .velocity_x = 1,
                                  .attack_cooldown_remaining = enemy->attack_cooldown_remaining};
    enemy->active = false;
}

/* Actions form a 3-by-3 grid: each axis is -1, 0, or +1; action 4 stays still.
 * The ship fires automatically, so Q-learning controls navigation only. */
static float step_game(GameState *game, int action) {

    if (game->done) {
        return 0;
    }
    assert(action >= 0 && action < ACTION_COUNT);
    float reward = 0.01f, previous_x = game->x, previous_y = game->y;
    int move_x = action % 3 - 1, move_y = action / 3 - 1;
    /* Normalize diagonal movement to preserve a speed of 400 pixels per second. */
    float speed = 400.0f * SIMULATION_TIME_STEP / ((move_x && move_y) ? sqrtf(2) : 1);
    game->x = clamp_float(game->x + move_x * speed, 25, WINDOW_WIDTH - 25);
    game->y = clamp_float(game->y + move_y * speed, 60, WINDOW_HEIGHT - 25);
    if ((move_x && game->x == previous_x) || (move_y && game->y == previous_y)) {
        reward -= 0.03f;
    }
    if (game->weapon_cooldown_ticks > 0) {
        game->weapon_cooldown_ticks--;
    }
    if (game->weapon_cooldown_ticks == 0) {
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!game->bullets[i].active) {
                game->bullets[i] = (Entity){.x = game->x, .y = game->y - 18, .active = true};
                game->weapon_cooldown_ticks = 8;
                game->shots++;
                break;
            }
        }
    }
    if ((game->spawn_timer -= SIMULATION_TIME_STEP) <= 0) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!game->enemies[i].active) {
                game->spawned++;
                int type = (int)(random_uint() % 3);
                game->enemies[i] = (Entity){.x = 25 + random_float() * (WINDOW_WIDTH - 50),
                                            .y = 35,
                                            .active = true,
                                            .type = type,
                                            .health = enemy_stats[type].health,
                                            .velocity_y = 10,
                                            .velocity_x = 200};
                Entity *enemy = &game->enemies[i];
                enemy->patrol_left = fmaxf(25, enemy->x - 175);
                enemy->patrol_right = fminf(WINDOW_WIDTH - 25, enemy->x + 175);
                break;
            }
        }
        game->spawn_timer += 0.75f;
    }
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (game->bullets[i].active) {
            Entity *entity = &game->bullets[i];
            entity->y -= 12;
            if (entity->y < 0) {
                entity->active = false;
            }
        }
    }
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (game->blasts[i].active) {
            Entity *entity = &game->blasts[i];
            entity->velocity_y -= SIMULATION_TIME_STEP;
            entity->y += entity->velocity_y * SIMULATION_TIME_STEP;
            int band = (int)floorf(entity->x) % 350;
            if (band > 0 && band < 10) {
                entity->velocity_x = -entity->velocity_x;
            }
            entity->x += entity->velocity_x * SIMULATION_TIME_STEP;
            if (entity->y > WINDOW_HEIGHT) {
                entity->active = false;
                continue;
            }
            if (entity->attack_cooldown_remaining > 0) {
                entity->attack_cooldown_remaining =
                    fmaxf(0, entity->attack_cooldown_remaining - SIMULATION_TIME_STEP);
            }
            float dx = game->x - entity->x, dy = game->y - entity->y;
            if (dx * dx + dy * dy < 75 * 75 && entity->attack_cooldown_remaining == 0) {
                entity->attack_cooldown_remaining = 0.15f;
                game->health--;
                game->hits++;
                reward -= 5;
                if (game->health <= 0) {
                    game->health = 0;
                    game->done = true;
                    return reward - 10;
                }
            }
        }
    }
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (game->enemies[i].active) {
            Entity *enemy = &game->enemies[i];
            float dx = game->x - enemy->x, dy = game->y - enemy->y;
            /* Fixed zigzag patrol: independent of ship and missile positions. */
            enemy->velocity_y += enemy_stats[enemy->type].acceleration * SIMULATION_TIME_STEP;
            enemy->y += enemy->velocity_y * SIMULATION_TIME_STEP;
            enemy->x += enemy->velocity_x * SIMULATION_TIME_STEP;
            if (enemy->patrol_right > enemy->patrol_left) {
                if (enemy->x >= enemy->patrol_right) {
                    enemy->x = 2 * enemy->patrol_right - enemy->x;
                    enemy->velocity_x = -fabsf(enemy->velocity_x);
                } else if (enemy->x <= enemy->patrol_left) {
                    enemy->x = 2 * enemy->patrol_left - enemy->x;
                    enemy->velocity_x = fabsf(enemy->velocity_x);
                }
            }
            if (enemy->y > WINDOW_HEIGHT + 30) {
                enemy->active = false;
                reward -= 1.0f;
                continue;
            }
            if (enemy->attack_cooldown_remaining > 0) {
                enemy->attack_cooldown_remaining =
                    fmaxf(0, enemy->attack_cooldown_remaining - SIMULATION_TIME_STEP);
            }

            for (int j = 0; j < MAX_BULLETS; j++) {
                if (game->bullets[j].active) {
                    float bx = enemy->x - game->bullets[j].x, by = enemy->y - game->bullets[j].y;
                    if (bx * bx + by * by < 35 * 35) {
                        game->bullets[j].active = false;
                        game->landed++;
                        reward += 0.15f;
                        if ((enemy->health -= 10) <= 0) {
                            create_explosion(game, enemy);
                            game->kills++;
                            game->score += enemy_stats[enemy->type].points;
                            reward += 12;
                            break;
                        }
                    }
                }
            }
            dx = game->x - enemy->x;
            dy = game->y - enemy->y;
            if (enemy->active && dx * dx + dy * dy < 50 * 50 &&
                enemy->attack_cooldown_remaining == 0) {
                enemy->attack_cooldown_remaining = enemy_stats[enemy->type].attack_cooldown_seconds;
                game->health -= (int)enemy_stats[enemy->type].damage;
                game->hits++;
                reward -= 5;
                if (game->health <= 0) {
                    reward -= 10;
                    game->done = true;
                    break;
                }
            }
        }
    }
    /* Dense feedback makes near misses matter, even with 1,000 health.
       Include all hazards so escaping one enemy into another is discouraged. */
    float danger = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (game->enemies[i].active) {
            Entity *enemy = &game->enemies[i];
            float dx = enemy->x - game->x, dy = enemy->y - game->y;
            float clearance = sqrtf(dx * dx + dy * dy) - 50;
            danger += clamp_float(1 - clearance / 140, 0, 1);
        }
    }
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (game->blasts[i].active) {
            Entity *entity = &game->blasts[i];
            float dx = entity->x - game->x, dy = entity->y - game->y;
            float clearance = sqrtf(dx * dx + dy * dy) - 75;
            danger += clamp_float(1 - clearance / 140, 0, 1);
        }
    }
    reward -= 0.025f * danger;
    /* Seek a firing lane below a live target, while outside contact range.
       Only the best lane pays, so crowds cannot multiply this bonus. */
    float firing_lane = 0;
    if (danger < 0.5f) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (game->enemies[i].active) {
                Entity *enemy = &game->enemies[i];
                float ahead = game->y - enemy->y;
                if (ahead > 100 && ahead < 450) {
                    float alignment = clamp_float(1 - fabsf(game->x - enemy->x) / 120, 0, 1);
                    firing_lane = fmaxf(firing_lane, alignment);
                }
            }
        }
    }
    reward += 0.04f * firing_lane;
    if (++game->tick >= MAX_EPISODE_STEPS) {
        game->done = true;
    }
    return reward;
}

static int choose_action(int state_index, float exploration_rate) {
    if (random_float() < exploration_rate) {
        return (int)(random_uint() % ACTION_COUNT);
    }
    int best = 0, n = 1;
    for (int action = 1; action < ACTION_COUNT; action++) {
        if (agent.q_values[state_index][action] > agent.q_values[state_index][best]) {
            best = action;
            n = 1;
        } else if (agent.q_values[state_index][action] == agent.q_values[state_index][best] &&
                   random_uint() % (uint32_t)++n == 0) {
            best = action;
        }
    }
    return best;
}

/* Move the selected action value toward reward + discounted best next value.
 * Terminal states have no future reward. */
static void update_q_value(int state_index, int action, float reward, int next, bool terminal) {
    float best = agent.q_values[next][0];
    for (int i = 1; i < ACTION_COUNT; i++) {
        if (agent.q_values[next][i] > best) {
            best = agent.q_values[next][i];
        }
    }
    float target = reward + (terminal ? 0 : 0.98f * best);
    agent.q_values[state_index][action] += 0.12f * (target - agent.q_values[state_index][action]);
}

static bool save_policy(void) {
    FILE *f = fopen(MODEL_PATH ".tmp", "wb");
    if (!f) {
        perror(MODEL_PATH);
        return false;
    }
    bool ok = fwrite("SRL2", 1, 4, f) == 4 && fwrite(&agent, sizeof agent, 1, f) == 1;
    if (fclose(f) != 0) {
        ok = false;
    }
    if (ok && rename(MODEL_PATH ".tmp", MODEL_PATH) == 0) {
        return true;
    }
    fprintf(stderr, "Could not save policy\n");
    return false;
}

static bool load_policy(void) {
    FILE *f = fopen(MODEL_PATH, "rb");
    if (!f) {
        return false;
    }
    char magic[4];
    bool ok = fread(magic, 1, 4, f) == 4 && memcmp(magic, "SRL2", 4) == 0 &&
              fread(&agent, sizeof agent, 1, f) == 1;
    fclose(f);
    if (ok) {
        for (int state_index = 0; state_index < STATE_COUNT && ok; state_index++) {
            for (int action = 0; action < ACTION_COUNT; action++) {
                if (!isfinite(agent.q_values[state_index][action])) {
                    ok = false;
                    break;
                }
            }
        }
    }
    if (!ok) {
        memset(&agent, 0, sizeof agent);
        fprintf(stderr, "Invalid model; starting fresh\n");
    }
    return ok;
}

static float exploration_rate(void) {
    return 0.05f + 0.95f * expf(-(float)agent.episodes / 2500);
}

static float run_episode(bool learn, bool random_policy, int *kills, int *hits, int *spawned) {
    GameState game;
    reset_game(&game);
    float total = 0;
    while (!game.done) {
        int state_index = encode_state(&game),
            action = random_policy ? (int)(random_uint() % ACTION_COUNT)
                                   : choose_action(state_index, learn ? exploration_rate() : 0);
        float reward = step_game(&game, action);
        if (learn) {
            update_q_value(state_index, action, reward, encode_state(&game), game.done);
        }
        total += reward;
    }
    *kills = game.kills;
    *hits = game.hits;
    *spawned = game.spawned;
    if (learn) {
        agent.episodes++;
    }
    return total;
}

static void evaluate_policy(int n, bool baseline) {
    random_seed = 987654321;
    double total = 0;
    int kills = 0, hits = 0, spawned = 0;
    for (int i = 0; i < n; i++) {
        int k, h, f;
        total += run_episode(false, baseline, &k, &h, &f);
        kills += k;
        hits += h;
        spawned += f;
    }
    printf("%s: %d episodes, reward %.2f, kills %.2f, hits %.2f (means), kill rate %.1f%%\n",
           baseline ? "Random" : "Learned", n, total / n, (double)kills / n, (double)hits / n,
           spawned ? 100.0 * kills / spawned : 0.0);
}

static void run_tests(void) {
    GameState left, right;
    reset_game(&left);
    reset_game(&right);
    left.x = 50;
    right.x = WINDOW_WIDTH - 50;
    left.spawn_timer = right.spawn_timer = 100;
    left.weapon_cooldown_ticks = right.weapon_cooldown_ticks = 100;
    left.enemies[0] = right.enemies[0] = (Entity){.x = 399,
                                                  .y = 100,
                                                  .active = true,
                                                  .health = 50,
                                                  .velocity_x = 200,
                                                  .velocity_y = 10,
                                                  .patrol_left = 50,
                                                  .patrol_right = 400};
    step_game(&left, 4);
    step_game(&right, 4);
    assert(left.enemies[0].x == right.enemies[0].x);
    assert(left.enemies[0].velocity_x < 0 && left.enemies[0].x <= 400);
    GameState spawned;
    reset_game(&spawned);
    step_game(&spawned, 4);
    assert(spawned.spawned == 1);
    step_game(&spawned, 4);
    assert(spawned.spawned == 1);
    GameState game;
    reset_game(&game);
    assert(encode_state(&game) >= 0 && encode_state(&game) < STATE_COUNT);
    game.spawn_timer = 100;
    game.weapon_cooldown_ticks = 100;
    game.enemies[0] = (Entity){.x = game.x, .y = game.y, .active = true, .health = 25};
    step_game(&game, 4);
    assert(game.health == START_HEALTH - 5 && game.hits == 1);
    reset_game(&game);
    game.spawn_timer = 100;
    game.weapon_cooldown_ticks = 100;
    game.enemies[0] = (Entity){.x = game.x, .y = game.y - 100, .active = true, .health = 1};
    game.bullets[0] = (Entity){.x = game.x, .y = game.y - 85, .active = true};
    step_game(&game, 4);
    assert(game.kills == 1 && !game.enemies[0].active && game.landed == 1);
    assert(game.blasts[0].active);
    reset_game(&game);
    game.spawn_timer = 100;
    game.weapon_cooldown_ticks = 100;
    game.blasts[0] = (Entity){.x = game.x, .y = game.y, .active = true, .lifetime_ticks = 36};
    step_game(&game, 4);
    assert(game.hits == 1 && game.health == START_HEALTH - 1);
    step_game(&game, 4);
    assert(game.hits == 1); /* explosion cooldown prevents per-frame damage */
    reset_game(&game);
    game.spawn_timer = 100;
    game.blasts[0] = (Entity){.y = WINDOW_HEIGHT + 1, .active = true};
    step_game(&game, 4);
    assert(!game.blasts[0].active);
    update_q_value(0, 0, 2, 1, true);
    assert(fabsf(agent.q_values[0][0] - 0.24f) < 0.0001f);
    memset(&agent, 0, sizeof agent);
    for (int i = 0; i < 100; i++) {
        reset_game(&game);
        while (!game.done) {
            int state_index = encode_state(&game);
            assert(state_index >= 0 && state_index < STATE_COUNT);
            step_game(&game, (int)(random_uint() % ACTION_COUNT));
        }
        assert(game.tick <= MAX_EPISODE_STEPS && game.health >= 0);
    }
    puts("PASS: damage, bullet kills, explosion damage/cooldown/expiry, terminal Q target, 100 "
         "simulations");
}

static Texture2D load_sprite(const char *name) {
    Texture2D t = LoadTexture(TextFormat("assets/%s.png", name));
    if (t.id) {
        SetTextureFilter(t, TEXTURE_FILTER_BILINEAR);
    }
    return t;
}

static void draw_sprite(Texture2D t, float x, float y, float width, float height, Color tint) {
    if (!t.id) {
        DrawCircle((int)x, (int)y, width / 2, tint);
        return;
    }
    DrawTexturePro(t, (Rectangle){0, 0, (float)t.width, (float)t.height},
                   (Rectangle){x, y, width, height}, (Vector2){width / 2, height / 2}, 0, tint);
}

static float interpolate(float previous, float current, float alpha) {
    return previous + (current - previous) * alpha;
}

static void draw_body(Texture2D texture, const Entity *previous, const Entity *current, float alpha,
                      float width, float height) {
    float dx = current->x - previous->x, dy = current->y - previous->y;
    /* New/reused slots appear at their spawn rather than streaking across the arena. */
    if (!previous->active || dx * dx + dy * dy > 100 * 100) {
        alpha = 1;
    }
    draw_sprite(texture, interpolate(previous->x, current->x, alpha),
                interpolate(previous->y, current->y, alpha), width, height, WHITE);
}

/* Accumulate real time into fixed simulation steps; interpolate drawing between them. */
static void run_window(void) {

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Spaceship RL - independent C project");
    SetTargetFPS(120);
    Texture2D ship = load_sprite("spaceship"), ammo = load_sprite("ammo1"),
              blast = load_sprite("explosion");
    Texture2D enemy_textures[3] = {load_sprite("aliencraft1"), load_sprite("aliencraft2"),
                                   load_sprite("aliencraft3")};
    GameState game;
    reset_game(&game);
    bool learning = false, manual = false, paused = false;
    GameState previous = game;
    int speed = 1;
    double accumulator = 0;
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_T)) {
            learning = !learning;
        }
        if (IsKeyPressed(KEY_M)) {
            manual = !manual;
        }
        if (IsKeyPressed(KEY_SPACE)) {
            paused = !paused;
        }
        if (IsKeyPressed(KEY_F)) {
            speed = speed == 1 ? 10 : 1;
        }
        if (IsKeyPressed(KEY_R)) {
            reset_game(&game);
            previous = game;
            accumulator = 0;
        }
        if (IsKeyPressed(KEY_S)) {
            save_policy();
        }
        accumulator += paused ? 0 : fmin(GetFrameTime(), 0.1) * speed;
        while (accumulator >= 1.0 / 30) {
            accumulator -= 1.0 / 30;
            if (game.done) {
                if (learning && !manual) {
                    agent.episodes++;
                }
                reset_game(&game);
            }
            previous = game;
            int state_index = encode_state(&game),
                action = choose_action(state_index, learning ? exploration_rate() : 0);
            if (manual) {
                int x = IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT),
                    y = IsKeyDown(KEY_DOWN) - IsKeyDown(KEY_UP);
                action = (y + 1) * 3 + x + 1;
            }
            float reward = step_game(&game, action);
            if (learning && !manual) {
                update_q_value(state_index, action, reward, encode_state(&game), game.done);
            }
        }
        float alpha = (float)(accumulator * 30);
        BeginDrawing();
        ClearBackground(BLACK);
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (game.enemies[i].active) {
                draw_body(enemy_textures[game.enemies[i].type], &previous.enemies[i],
                          &game.enemies[i], alpha, 50, 50);
            }
        }
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (game.bullets[i].active) {
                draw_body(ammo, &previous.bullets[i], &game.bullets[i], alpha, 20, 30);
            }
        }
        for (int i = 0; i < MAX_EXPLOSIONS; i++) {
            if (game.blasts[i].active) {
                draw_body(blast, &previous.blasts[i], &game.blasts[i], alpha, 100, 100);
            }
        }
        draw_sprite(ship, interpolate(previous.x, game.x, alpha),
                    interpolate(previous.y, game.y, alpha), 50, 50, WHITE);

        DrawRectangle(0, 0, WINDOW_WIDTH, 54, (Color){15, 24, 44, 255});
        DrawText(TextFormat("%s  HP %d   Kills %d   Score %d   Kill rate %.1f%%   Ep %u   %dx",
                            manual     ? "MANUAL"
                            : learning ? "TRAINING"
                                       : "POLICY",
                            game.health, game.kills, game.score,
                            game.spawned ? 100.0 * game.kills / game.spawned : 0.0, agent.episodes,
                            speed),
                 16, 8, 20, RAYWHITE);
        DrawText("T train | M manual | arrows move | F speed | Space pause | R reset | S save", 16,
                 34, 14, GRAY);
        EndDrawing();
    }
    save_policy();
    UnloadTexture(ship);
    UnloadTexture(ammo);
    UnloadTexture(blast);
    for (int i = 0; i < 3; i++) {
        UnloadTexture(enemy_textures[i]);
    }
    CloseWindow();
}
int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--test") == 0) {
        run_tests();
        return 0;
    }
    load_policy();
    if (argc == 3 && (strcmp(argv[1], "--train") == 0 || strcmp(argv[1], "--eval") == 0)) {
        char *end;
        long n = strtol(argv[2], &end, 10);
        if (*end || n < 1 || n > 1000000) {
            fprintf(stderr, "Episode count must be 1..1000000\n");
            return 1;
        }
        if (strcmp(argv[1], "--eval") == 0) {
            evaluate_policy((int)n, true);
            evaluate_policy((int)n, false);
            return 0;
        }
        double total = 0;
        for (long i = 0; i < n; i++) {
            int k, h, f;
            total += run_episode(true, false, &k, &h, &f);
            if ((i + 1) % 1000 == 0) {
                printf("Episodes %u | epsilon %.3f | mean reward %.2f\n", agent.episodes,
                       exploration_rate(), total / 1000);
                total = 0;
            }
        }
        return save_policy() ? 0 : 1;
    }
    if (argc != 1) {
        fprintf(stderr, "Usage: %s [--train N | --eval N | --test]\nRun from AI_Project.\n",
                argv[0]);
        return 1;
    }
    run_window();
    return 0;
}
