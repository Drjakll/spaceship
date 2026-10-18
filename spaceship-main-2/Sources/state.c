#include "game.h"

#include <stdlib.h>

/** @copydoc game_initialize_agents */
void game_initialize_agents(Game_State *game) {
  for (int i = 0; i < NUM_OF_AGENTS; i++) {
    Texture model = assets_load_spaceship_model();
    Spaceship *ship = spaceship_create(model, game->assets.ammo_model);
    ship->position = (Vector2){400 + 50 * i, 700};
    game->spaceships[i] = ship;
  }
}

/** @copydoc game_add_enemy */
void game_add_enemy(Game_State *game, Enemy *enemy) {
  if (enemy != NULL) {
    enemy_collection_insert(game->enemies, enemy);
  }
}

/** @copydoc game_add_projectile */
void game_add_projectile(Game_State *game, Ammo *ammo) {
  if (ammo != NULL) {
    ammo->id = ++game->projectile_id;
    projectile_collection_insert(game->projectiles, ammo);
  }
}

/** @copydoc game_release_entities */
void game_release_entities(Game_State *game) {
  Enemy_Node *enemy_node = game->enemies->head;
  while (enemy_node != NULL) {
    Enemy_Node *next = enemy_node->next;
    enemy_destroy(enemy_node->enemy);
    free(enemy_node);
    enemy_node = next;
  }
  free(game->enemies);
  game->enemies = NULL;

  Projectile_Node *projectile_node = game->projectiles->head;
  while (projectile_node != NULL) {
    Projectile_Node *next = projectile_node->next;
    projectile_destroy(projectile_node->ammo);
    free(projectile_node);
    projectile_node = next;
  }
  free(game->projectiles);
  game->projectiles = NULL;

  for (int i = 0; i < NUM_OF_AGENTS; i++) {
    Spaceship *ship = game->spaceships[i];
    free(ship->weapon);
    free(ship);
    game->spaceships[i] = NULL;
  }
}
