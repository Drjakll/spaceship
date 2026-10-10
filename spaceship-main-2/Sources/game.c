#include "game.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/** @copydoc movement_normalize */
Vector2 movement_normalize(Vector2 direction, float speed) {
  float magnitude = sqrt(direction.x * direction.x +
                         direction.y * direction.y);
  if (magnitude == 0) {
    return direction;
  }
  direction.x = speed * direction.x / magnitude;
  direction.y = speed * direction.y / magnitude;
  return direction;
}

/** @copydoc game_control_spaceship */
void game_control_spaceship(Game_State *game, int agent, int actions,
                            double delta_time) {
  Spaceship *ship = game->spaceships[agent];
  Vector2 direction = {0, 0};
  if (actions & LEFT) {
    direction.x -= 1;
  }
  if (actions & RIGHT) {
    direction.x += 1;
  }
  if (actions & UP) {
    direction.y -= 1;
  }
  if (actions & DOWN) {
    direction.y += 1;
  }

  Vector2 velocity = movement_normalize(direction, ship->speed);
  ship->position.x += velocity.x * delta_time;
  ship->position.y += velocity.y * delta_time;

  if (ship->position.x > WINDOW_WIDTH - SPACESHIP_RADIUS) {
    ship->position.x = WINDOW_WIDTH - SPACESHIP_RADIUS;
  } else if (ship->position.x < 0) {
    ship->position.x = 0;
  }
  if (ship->position.y > WINDOW_HEIGHT - SPACESHIP_RADIUS) {
    ship->position.y = WINDOW_HEIGHT - SPACESHIP_RADIUS;
  } else if (ship->position.y < 0) {
    ship->position.y = 0;
  }

  Weapon *weapon = ship->weapon;
  weapon->cooldown_remaining -= delta_time;
  if (actions & SHOOT) {
    Ammo *ammo = weapon->shoot(weapon->cooldown_remaining, ship->position,
                               agent, weapon->projectile_model);
    if (ammo != NULL) {
      weapon->cooldown_remaining = weapon->cooldown;
      game_add_projectile(game, ammo);
    }
  }
}

/** @copydoc game_initialize */
void game_initialize(Game_State *game) {
  assets_load(&game->assets);
  SetTargetFPS(TARGET_FPS);
  game->projectiles = calloc(1, sizeof(*game->projectiles));
  game->enemies = calloc(1, sizeof(*game->enemies));
  game->spawn_timer = ENEMY_SPAWN_TIMER;
  game->previous_time = GetTime();
  game->font = LoadFont("Fonts/NotoSans-VariableFont.ttf");
  game_initialize_agents(game);
}

/**
 * @brief Draw all initialized spaceships at their current screen positions.
 * @param game Initialized state supplying ship textures and positions.
 * @pre Called inside an active raylib drawing frame.
 */
static void draw_spaceships(const Game_State *game) {
  for (int i = 0; i < NUM_OF_AGENTS; i++) {
    const Spaceship *ship = game->spaceships[i];
    DrawTexture(ship->model, ship->position.x, ship->position.y, WHITE);
  }
}

/**
 * @brief Draw each agent's score and the inherited game-time label.
 * @param game Initialized state supplying scores, accumulated time, and the
 * font.
 * @pre Called inside an active raylib drawing frame.
 * @note The active game loop does not advance accumulated_time.
 */
static void draw_status(const Game_State *game) {
  char score_text[100];
  for (int i = 0; i < NUM_OF_AGENTS; i++) {
    snprintf(score_text, sizeof(score_text), "Total Score: %d",
             game->scores[i]);
    DrawTextEx(game->font, score_text, (Vector2){50, 50 + 50 * i},
               24, 1, WHITE);
  }

  char time_text[25];
  snprintf(time_text, sizeof(time_text), "Time: %d",
           GAME_TIME - (int)game->accumulated_time);
  DrawTextEx(game->font, time_text, (Vector2){WINDOW_WIDTH / 2, 50},
             24, 1, WHITE);
}

/**
 * @brief Render one frame while running collision and motion visitors in order.
 * @param game Initialized state whose entity collections and scores may change.
 * @param delta_time Elapsed simulation time in seconds passed to the visitors.
 * @pre A raylib window is open and no drawing frame is currently active.
 * @note Owns BeginDrawing()/EndDrawing() and preserves the original frame
 * phases.
 */
static void draw_and_simulate(Game_State *game, double delta_time) {
  Collision_Context collision = {
    .enemies = game->enemies,
    .scores = game->scores,
    .explosion_model = game->assets.explosion_model
  };

  /* Preserve the original frame phases: ships, hits, enemies, projectiles. */
  BeginDrawing();
  ClearBackground(BLACK);
  draw_spaceships(game);
  projectile_collection_visit(game->projectiles, collision_projectile,
                              delta_time, &collision);
  enemy_collection_visit(game->enemies, enemy_move_and_draw,
                          delta_time, NULL);
  enemy_collection_visit(game->enemies, enemy_update_attack_cooldown,
                          delta_time, NULL);
  projectile_collection_visit(game->projectiles, projectile_move_and_draw,
                              delta_time, NULL);
  draw_status(game);
  EndDrawing();
}

/** @copydoc game_frame */
void game_frame(Game_State *game, double current_time) {
  double delta_time = current_time - game->previous_time;
  game->spawn_timer -= delta_time;

  /* Autonomous random control is the existing behavior. */
  for (int i = 0; i < NUM_OF_AGENTS; i++) {
    int actions = GetRandomValue(NO_ACTION, ALL_ACTIONS);
    game_control_spaceship(game, i, actions, delta_time);
  }
  game->previous_time = current_time;

  if (game->spawn_timer <= 0.0f) {
    Enemy_Variant variant = GetRandomValue(0, ENEMY_VARIANT_COUNT - 1);
    Enemy *enemy = enemy_create(variant, game->assets.enemy_models[variant]);
    game_add_enemy(game, enemy);
    game->spawn_timer = ENEMY_SPAWN_TIMER;
  }

  draw_and_simulate(game, delta_time);
  game->frame_count++;
}

/** @copydoc game_shutdown */
void game_shutdown(Game_State *game) {
  assets_unload(&game->assets);
  for (int i = 0; i < NUM_OF_AGENTS; i++) {
    UnloadTexture(game->spaceships[i]->model);
  }
  UnloadFont(game->font);
  game_release_entities(game);
  CloseWindow();
}

/** @copydoc game_run */
int game_run(void) {
  Game_State game = {0};
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Spaceship");
  game_initialize(&game);
  while (!WindowShouldClose()) {
    game_frame(&game, GetTime());
  }
  game_shutdown(&game);
  return 0;
}
