#ifndef SPACESHIP_GAME_H
#define SPACESHIP_GAME_H

#include "assets.h"
#include "detections.h"

/* One owner for mutable simulation state; no process-wide game globals. */
typedef struct Game_State {
  Game_Assets assets;
  Enemy_Collections *enemies;
  Projectile_Collections *projectiles;
  Spaceship *spaceships[NUM_OF_AGENTS];
  int scores[NUM_OF_AGENTS];
  int frame_count;
  long projectile_id;
  float spawn_timer;
  double previous_time;
  /* Recording/time-limit logic was disabled in the original game. */
  double accumulated_time;
  Font font;
} Game_State;

/* Call with an open raylib window and a zero-initialized state. */
/**
 * @brief Load game resources and prepare collections, timing, and agents.
 * @param game Zero-initialized state that takes ownership of loaded resources.
 * @pre A raylib window is open; the project root is the working directory.
 * @note Call once per state, then release resources with game_shutdown().
 */
void game_initialize(Game_State *game);

/**
 * @brief Create the configured spaceships at their original starting positions.
 * @param game State receiving the ships and their individually loaded textures.
 * @pre Shared assets are loaded, ship slots are empty, and a window is open.
 * @note This replaces every ship slot; it does not release previous occupants.
 */
void game_initialize_agents(Game_State *game);

/**
 * @brief Advance and draw one frame using the existing random agent controls.
 * @param game Initialized simulation state to update.
 * @param current_time Absolute time in seconds, using the GetTime() clock.
 * @pre The game is initialized and the raylib window remains open.
 * @note Updates timing, spawn state, entities, scores, and the frame counter.
 * @note Recording and time-limit updates remain inactive.
 */
void game_frame(Game_State *game, double current_time);

/**
 * @brief Apply movement flags, boundary clamping, and missile firing to one
 * ship.
 * @param game State owning the target ship and projectile collection.
 * @param agent Ship index in the range [0, NUM_OF_AGENTS).
 * @param actions Bitmask combining UP, DOWN, LEFT, RIGHT, and SHOOT.
 * @param delta_time Elapsed simulation time in seconds.
 * @pre The selected ship, its weapon, and the projectile collection exist.
 * @note Opposing directions cancel; diagonal movement is normalized to speed.
 */
void game_control_spaceship(Game_State *game, int agent, int actions,
                            double delta_time);

/**
 * @brief Scale a nonzero direction vector to the requested movement speed.
 * @param direction Direction to normalize without modifying the caller's value.
 * @param speed Magnitude to assign to a nonzero direction.
 * @return The scaled vector, or the unchanged zero vector when no direction
 * exists.
 */
Vector2 movement_normalize(Vector2 direction, float speed);

/* The game takes ownership of inserted entities. */
/**
 * @brief Append an enemy to the game's owned enemy collection.
 * @param game State with an initialized enemy collection.
 * @param enemy Entity whose ownership transfers to the game; NULL is ignored.
 */
void game_add_enemy(Game_State *game, Enemy *enemy);

/**
 * @brief Assign a new projectile ID and append the projectile to the game.
 * @param game State with an initialized projectile collection and ID counter.
 * @param ammo Entity whose ownership transfers to the game; NULL is ignored.
 * @note IDs increase within this game state rather than across all game
 * instances.
 */
void game_add_projectile(Game_State *game, Ammo *ammo);

/**
 * @brief Free owned entities, weapons, velocities, collection nodes, and lists.
 * @param game Initialized state whose entity allocations will be released.
 * @pre Both collections and all spaceship slots exist; call only once.
 * @note Clears entity pointers, but does not unload textures, images, or the
 * font.
 */
void game_release_entities(Game_State *game);

/**
 * @brief Unload game resources, free entities, and close the raylib window.
 * @param game Fully initialized state to shut down.
 * @pre Its resources remain loaded and its entity allocations remain valid.
 * @note Call once after the game loop; the state cannot be simulated afterward.
 */
void game_shutdown(Game_State *game);

/**
 * @brief Open the game window and run frames until raylib requests closure.
 * @return Zero after resources are released and the window is closed.
 * @pre The project root is the working directory so asset paths resolve.
 */
int game_run(void);

#endif
