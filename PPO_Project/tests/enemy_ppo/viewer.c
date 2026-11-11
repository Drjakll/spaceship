#include <raylib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "enemy_ppo.h"

typedef struct {
    Texture2D ship;
    Texture2D enemy[3];
    Texture2D explosion;
    Texture2D missile;
    Font font;
} Art;

static Texture2D load_sprite(const char *assets, const char *filename, int width, int height) {
    char path[4096];
    if (snprintf(path, sizeof path, "%s/Pictures/%s", assets, filename) >= (int)sizeof path) {
        fprintf(stderr, "enemy viewer: asset path too long\n");
        exit(1);
    }
    Image image = LoadImage(path);
    if (!image.data) {
        fprintf(stderr, "enemy viewer: cannot load %s\n", path);
        exit(1);
    }
    ImageResize(&image, width, height);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    if (!texture.id) {
        fprintf(stderr, "enemy viewer: could not create texture for %s\n", path);
        exit(1);
    }
    return texture;
}

static Art load_art(const char *assets) {
    Art art = {0};
    art.ship = load_sprite(assets, "spaceship.png", (int)(2.0f * SHIP_RADIUS), (int)(2.0f * SHIP_RADIUS));
    art.enemy[0] = load_sprite(assets, "aliencraft1.png", (int)(2.0f * ENEMY_RADIUS), (int)(2.0f * ENEMY_RADIUS));
    art.enemy[1] = load_sprite(assets, "aliencraft2.png", (int)(2.0f * ENEMY_RADIUS), (int)(2.0f * ENEMY_RADIUS));
    art.enemy[2] = load_sprite(assets, "aliencraft3.png", (int)(2.0f * ENEMY_RADIUS), (int)(2.0f * ENEMY_RADIUS));
    art.explosion =
        load_sprite(assets, "explosion.png", (int)(2.0f * EXPLOSION_RADIUS), (int)(2.0f * EXPLOSION_RADIUS));
    art.missile = load_sprite(assets, "ammo1.png", (int)(2.0f * MISSILE_RADIUS), (int)(3.0f * MISSILE_RADIUS));
    char path[4096];
    if (snprintf(path, sizeof path, "%s/Fonts/NotoSans-VariableFont.ttf", assets) >= (int)sizeof path) {
        fprintf(stderr, "enemy viewer: font path too long\n");
        exit(1);
    }
    art.font = LoadFontEx(path, 22, NULL, 0);
    if (!art.font.texture.id) {
        art.font = GetFontDefault();
    }
    return art;
}

static void unload_art(Art *art) {
    UnloadTexture(art->ship);
    for (int i = 0; i < 3; i++) {
        UnloadTexture(art->enemy[i]);
    }
    UnloadTexture(art->explosion);
    UnloadTexture(art->missile);
    if (art->font.texture.id && art->font.texture.id != GetFontDefault().texture.id) {
        UnloadFont(art->font);
    }
}

static void draw_scene(const Env *env, const Art *art, int seed, int paused, int speed, int done, int escaped,
                       int killed) {
    ClearBackground(BLACK);
    for (int i = 0; i < env->n_enemies; i++) {
        const Enemy *enemy = &env->enemies[i];
        DrawTexture(enemy->dead ? art->explosion : art->enemy[enemy->type], (int)enemy->x, (int)enemy->y, WHITE);
    }
    for (int i = 0; i < env->n_missiles; i++) {
        DrawTexture(art->missile, (int)env->missiles[i].x, (int)env->missiles[i].y, WHITE);
    }
    for (int i = 0; i < NUM_SHIPS; i++) {
        const Ship *ship = &env->ships[i];
        if (ship->alive) {
            DrawTexture(art->ship, (int)ship->x, (int)ship->y, WHITE);
        }
    }

    DrawRectangle(8, 8, 600, 110, (Color){0, 0, 0, 190});
    DrawTextEx(art->font, "Learned enemy PPO vs frozen ship PPO", (Vector2){16, 14}, 22, 1, RAYWHITE);
    char line[128];
    int alive = 0, team_score = 0;
    for (int i = 0; i < NUM_SHIPS; i++) {
        alive += env->ships[i].alive;
        team_score += env->stats.score[i];
    }
    snprintf(line, sizeof line, "Time %.1f / %d s   seed %d   ships alive %d/3",
             (double)env->tick / TICK_HZ, ENEMY_VIEW_EPISODE_SECONDS, seed, alive);
    DrawTextEx(art->font, line, (Vector2){16, 42}, 18, 1, RAYWHITE);
    snprintf(line, sizeof line, "Team score %d   kills %d   escaped %d", team_score,
             env->stats.kills[0] + env->stats.kills[1] + env->stats.kills[2], env->stats.escapes);
    DrawTextEx(art->font, line, (Vector2){16, 66}, 18, 1, RAYWHITE);
    for (int i = 0; i < NUM_SHIPS; i++) {
        snprintf(line, sizeof line, "Ship %d: %s  HP %.0f  ammo %d", i,
                 env->ships[i].alive ? "alive" : "destroyed", env->ships[i].health, env->ships[i].ammo);
        DrawTextEx(art->font, line, (Vector2){16 + 220.0f * i, 94}, 16, 1, RAYWHITE);
    }
    const char *status = done ? (escaped ? "Enemy escaped; episode complete.  R replay  N next seed  Esc quit"
                                         : killed ? "Enemy destroyed; episode complete.  R replay  N next seed  Esc quit"
                                                  : "Episode complete.  R replay  N next seed  Esc quit")
                              : escaped ? "Learned enemy escaped; episode continues"
                              : killed ? "Learned enemy destroyed; episode continues"
                              : paused  ? "Paused (Space)"
                                        : NULL;
    if (status) {
        DrawTextEx(art->font, status, (Vector2){16, 132}, 20, 1, YELLOW);
    }
    DrawTextEx(art->font, TextFormat("Space pause   F speed %dx   R replay   N next seed   Esc quit", speed),
               (Vector2){16, ARENA_SIZE - 30}, 18, 1, LIGHTGRAY);
}

int main(int argc, char **argv) {
    const char *enemy_checkpoint = NULL;
    const char *ship_checkpoint = NULL;
    const char *assets = "../../../";
    uint64_t seed = 10000;
    for (int i = 1; i < argc; i++) {
        const char *flag = argv[i];
        if (i + 1 >= argc) {
            fprintf(stderr, "missing value for %s\n", flag);
            return 2;
        }
        const char *value = argv[++i];
        if (!strcmp(flag, "--enemy-checkpoint")) {
            enemy_checkpoint = value;
        } else if (!strcmp(flag, "--ship-checkpoint")) {
            ship_checkpoint = value;
        } else if (!strcmp(flag, "--assets")) {
            assets = value;
        } else if (!strcmp(flag, "--seed")) {
            seed = strtoull(value, NULL, 10);
        } else {
            fprintf(stderr, "unknown option %s\n", flag);
            return 2;
        }
    }
    if (!enemy_checkpoint || !ship_checkpoint) {
        fputs("usage: enemy_ppo_viewer --enemy-checkpoint FILE --ship-checkpoint FILE [--seed N] [--assets DIR]\n",
              stderr);
        return 2;
    }

    EnemyPpoPlayer player = {0};
    if (enemy_ppo_player_init(&player, enemy_checkpoint, ship_checkpoint, seed) != 0) {
        return 1;
    }
    SetTraceLogLevel(LOG_WARNING);
    InitWindow((int)ARENA_SIZE, (int)ARENA_SIZE, "Learned enemy PPO vs PPO ships");
    SetTargetFPS(TICK_HZ / ACTION_REPEAT);
    Art art = load_art(assets);
    int paused = 0, speed = 1, done = 0, escaped = 0, killed = 0;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_N)) {
            if (IsKeyPressed(KEY_N)) {
                seed++;
            }
            enemy_ppo_player_reset(&player);
            done = escaped = killed = 0;
        }
        if (IsKeyPressed(KEY_SPACE)) {
            paused = !paused;
        }
        if (IsKeyPressed(KEY_F)) {
            speed = speed == 1 ? 4 : 1;
        }
        for (int i = 0; i < speed && !paused && !done; i++) {
            int rc = enemy_ppo_player_step(&player);
            if (rc < 0) {
                done = 1;
                break;
            }
            done = rc > 0;
            enemy_ppo_player_done(&player, &escaped, &killed);
        }
        BeginDrawing();
        draw_scene(enemy_ppo_player_env(&player), &art, (int)seed, paused, speed, done, escaped, killed);
        EndDrawing();
    }

    unload_art(&art);
    CloseWindow();
    enemy_ppo_player_free(&player);
    return 0;
}
