#include "detections.h"

#include <math.h>
#include <stdlib.h>

/** @copydoc vector_distance */
float vector_distance(Vector2 first, Vector2 second) {
  Vector2 difference = {second.x - first.x, second.y - first.y};
  return sqrt(difference.x * difference.x + difference.y * difference.y);
}

/** @copydoc enemy_ship_displacement */
float *enemy_ship_displacement(Enemy enemy, Spaceship spaceship) {
  float *difference = malloc(2 * sizeof(*difference));
  difference[0] = spaceship.position.x - enemy.position.x;
  difference[1] = spaceship.position.y - enemy.position.y;
  return difference;
}

/** @copydoc enemy_detect_locations */
bool enemy_detect_locations(Enemy *enemy, double delta_time, void *context) {
  (void)enemy;
  (void)delta_time;
  (void)context;
  return false;
}

typedef struct Projectile_Collision {
  Ammo *ammo;
  Collision_Context *world;
} Projectile_Collision;

/**
 * @brief Configure a defeated enemy as the existing drifting, damaging
 * explosion.
 * @param enemy Mutable enemy whose model, motion, radius, and damage are
 * replaced.
 * @param model Borrowed explosion texture shared with other defeated enemies.
 * @note Marks the enemy dead without resetting its remaining contact cooldown.
 */
static void turn_into_explosion(Enemy *enemy, Texture model) {
  enemy->model = model;
  enemy->velocity->y = 50.0f;
  enemy->velocity->x = 1.0f;
  enemy->acceleration = 0.0f;
  enemy->radius = EXPLOSION_RADIUS;
  enemy->damage = 1.0f;
  enemy->attack_cooldown = EXPLOSION_DMG_COOLDOWN;
  enemy->dead = true;
}

/**
 * @brief Apply one projectile's damage to an overlapping enemy that is not
 * dead.
 * @param enemy Mutable enemy to test and potentially convert into an explosion.
 * @param delta_time Unused elapsed time, retained for visitor compatibility.
 * @param context Borrowed Projectile_Collision containing projectile and world
 * state.
 * @return True when damage is applied; false for a miss or an already-dead
 * enemy.
 * @note Only lethal hits award points to the projectile's owner.
 */
static bool collide_with_enemy(Enemy *enemy, double delta_time,
                               void *context) {
  (void)delta_time;
  Projectile_Collision *collision = context;
  Ammo *ammo = collision->ammo;
  float distance = vector_distance(ammo->position, enemy->position);
  float collision_radius = ammo->radius + enemy->radius;
  if (!(distance < collision_radius) || enemy->dead) {
    return false;
  }

  enemy->health -= ammo->damage;
  if (enemy->health <= 0.0f) {
    turn_into_explosion(enemy, collision->world->explosion_model);
    collision->world->scores[ammo->owner] += enemy->points_worth;
  }
  return true;
}

/** @copydoc collision_projectile */
bool collision_projectile(Ammo *ammo, double delta_time,
                          void *context) {
  if (ammo == NULL) {
    return false;
  }
  Projectile_Collision collision = {.ammo = ammo, .world = context};
  return enemy_collection_visit(collision.world->enemies, collide_with_enemy,
                                 delta_time, &collision);
}

/** @copydoc enemy_move_and_draw */
bool enemy_move_and_draw(Enemy *enemy, double delta_time, void *context) {
  (void)context;
  enemy->flight_path(&enemy->position, delta_time, enemy->velocity,
                    enemy->acceleration);
  DrawTexture(enemy->model, enemy->position.x, enemy->position.y, WHITE);
  return false;
}

/** @copydoc projectile_move_and_draw */
bool projectile_move_and_draw(Ammo *ammo, double delta_time, void *context) {
  (void)context;
  ammo->trajectory(&ammo->position, delta_time, ammo->velocity,
                   ammo->acceleration);
  DrawTexture(ammo->model, ammo->position.x, ammo->position.y, WHITE);
  return false;
}

/** @copydoc enemy_update_attack_cooldown */
bool enemy_update_attack_cooldown(Enemy *enemy, double delta_time,
                                 void *context) {
  (void)context;
  enemy->attack_cooldown_remaining -= delta_time;
  return false;
}

/** @copydoc collision_spaceship */
bool collision_spaceship(Enemy *enemy, Spaceship *spaceship) {
  float distance = vector_distance(spaceship->position, enemy->position);
  float collision_radius = spaceship->radius + enemy->radius;
  if (distance < collision_radius && enemy->attack_cooldown_remaining <= 0.0f) {
    spaceship->health -= enemy->damage;
    enemy->attack_cooldown_remaining = enemy->attack_cooldown;
  }
  return spaceship->health <= 0.0f;
}
