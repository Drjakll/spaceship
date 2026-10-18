#ifndef SPACESHIP_OBJECTS_H
#define SPACESHIP_OBJECTS_H

#include <stdbool.h>
#include <raylib.h>
#include "types.h"
#include "macros.h"

/* Motion callbacks update both position and velocity in place. */
typedef void (*Motion_Path)(Vector2 *, float, Vector2 *, float);
typedef Ammo *(*Weapon_Shoot)(float, Vector2, int, Texture);

struct Ammo {
  Vector2 position;
  float damage;
  Texture model;
  Motion_Path trajectory;
  float damage_area_radius;
  Vector2 *velocity;
  float acceleration;
  float radius;
  int owner;
  long id;
};

struct Weapon {
  float cooldown;
  float cooldown_remaining;
  Weapon_Shoot shoot;
  Texture projectile_model;
};

struct Enemy {
  const char *name;
  Vector2 position;
  int points_worth;
  Vector2 *velocity;
  float acceleration;
  float health;
  float damage;
  Texture model;
  Motion_Path flight_path;
  float radius;
  bool dead;
  float attack_cooldown;
  float attack_cooldown_remaining;
};

struct Spaceship {
  Vector2 position;
  Weapon *weapon;
  float health;
  float speed;
  Texture model;
  float radius;
};

/**
 * @brief Accelerate vertical velocity and move toward decreasing screen y.
 * @param position Mutable position advanced using the updated vertical
 * velocity.
 * @param delta_time Elapsed simulation time in seconds.
 * @param velocity Mutable velocity whose y component gains acceleration.
 * @param acceleration Vertical acceleration in screen units per second squared.
 * @note Horizontal position and velocity are unchanged.
 */
void motion_accelerate_up(Vector2 *position, float delta_time,
                         Vector2 *velocity, float acceleration);

/**
 * @brief Accelerate downward while moving horizontally through zigzag turn
 * bands.
 * @param position Mutable screen position to advance.
 * @param delta_time Elapsed simulation time in seconds.
 * @param velocity Mutable velocity whose x direction may reverse.
 * @param acceleration Vertical acceleration in screen units per second squared.
 * @note Horizontal velocity reverses when floored x is 1 through 9 modulo 350.
 */
void motion_accelerate_zigzag(Vector2 *position, float delta_time,
                             Vector2 *velocity, float acceleration);
typedef enum Enemy_Variant {
  ENEMY_VARIANT_1,
  ENEMY_VARIANT_2,
  ENEMY_VARIANT_3
} Enemy_Variant;

/**
 * @brief Create an enemy from the selected variant's shared definition.
 * @param variant Valid ENEMY_VARIANT_1, ENEMY_VARIANT_2, or ENEMY_VARIANT_3
 * value.
 * @param model Borrowed texture to draw; the factory does not own the texture.
 * @return A heap-owned enemy with an owned velocity; release with
 * enemy_destroy().
 * @pre Entity and velocity allocations succeed.
 * @note Consumes one random value for the horizontal spawn position.
 */
Enemy *enemy_create(Enemy_Variant variant, Texture model);

/**
 * @brief Create a missile when the weapon cooldown permits firing.
 * @param cooldown Remaining weapon cooldown in seconds.
 * @param position Initial screen position of the missile.
 * @param owner Agent index used to attribute collision scores.
 * @param model Borrowed texture used to draw the missile.
 * @return An owned missile, or NULL when cooldown is greater than zero.
 * @pre When firing, entity and velocity allocations succeed.
 * @note Does not assign an ID; game_add_projectile() assigns the ID on
 * insertion.
 */
Ammo *projectile_create(float cooldown, Vector2 position, int owner,
                        Texture model);

/**
 * @brief Allocate a spaceship with the default missile weapon and movement
 * stats.
 * @param model Borrowed spaceship texture; texture lifetime stays with the
 * caller.
 * @param projectile_model Borrowed texture passed to the default weapon's
 * shots.
 * @return An owned spaceship with an owned weapon; the game releases both.
 * @pre Spaceship and weapon allocations succeed.
 */
Spaceship *spaceship_create(Texture model, Texture projectile_model);

/**
 * @brief Release an enemy and its owned velocity allocation.
 * @param enemy Non-NULL enemy that is no longer referenced by a live
 * collection.
 * @note The borrowed model texture is not unloaded.
 */
void enemy_destroy(Enemy *enemy);

/**
 * @brief Release a projectile and its owned velocity allocation.
 * @param ammo Non-NULL projectile no longer referenced by a live collection.
 * @note The borrowed model texture is not unloaded.
 */
void projectile_destroy(Ammo *ammo);

#endif
