#ifndef SPACESHIP_DETECTIONS_H
#define SPACESHIP_DETECTIONS_H

#include "object_collections.h"

/* Collision visitors receive this context rather than shared scratch state. */
typedef struct Collision_Context {
  Enemy_Collections *enemies;
  int *scores;
  Texture explosion_model;
} Collision_Context;

/**
 * @brief Compute the Euclidean distance between two screen positions.
 * @param first First position.
 * @param second Second position.
 * @return Nonnegative distance measured in the positions' coordinate units.
 */
float vector_distance(Vector2 first, Vector2 second);
/* Caller owns the returned two-component displacement. */
/**
 * @brief Calculate displacement from an enemy's position to a ship's position.
 * @param enemy Enemy providing the displacement origin.
 * @param spaceship Ship providing the displacement destination.
 * @return Two allocated floats, x then y; the caller must free the array.
 * @pre The displacement-array allocation succeeds.
 */
float *enemy_ship_displacement(Enemy enemy, Spaceship spaceship);

/**
 * @brief Preserve the inactive enemy-location visitor used by control
 * scaffolding.
 * @param enemy Unused enemy argument.
 * @param delta_time Unused elapsed-time argument.
 * @param context Unused callback context.
 * @return Always false; no state is inspected or modified.
 */
bool enemy_detect_locations(Enemy *enemy, double delta_time, void *context);

/**
 * @brief Apply projectile damage to every overlapping live enemy.
 * @param ammo Projectile being tested; NULL produces no collision.
 * @param delta_time Elapsed seconds forwarded to the enemy collection visitor.
 * @param context Borrowed Collision_Context with enemies, scores, and
 * explosion.
 * @return True if an invoked enemy check applies damage; otherwise false.
 * @pre Non-NULL ammo has a valid owner index and context points to valid state.
 * @note Lethal hits turn enemies into explosions and award the owner's score.
 * @note Enemy traversal also prunes offscreen enemies; no early hit exit is
 * used.
 */
bool collision_projectile(Ammo *ammo, double delta_time, void *context);

/**
 * @brief Advance an enemy along its flight path, then draw its updated
 * position.
 * @param enemy Live enemy with a flight-path callback and valid velocity.
 * @param delta_time Elapsed simulation time in seconds.
 * @param context Unused visitor context.
 * @return Always false, so movement alone does not report a collision.
 * @pre Called inside a raylib drawing frame.
 */
bool enemy_move_and_draw(Enemy *enemy, double delta_time, void *context);

/**
 * @brief Advance a projectile's trajectory, then draw its updated position.
 * @param ammo Live projectile with a trajectory callback and valid velocity.
 * @param delta_time Elapsed simulation time in seconds.
 * @param context Unused visitor context.
 * @return Always false, so this visitor does not mark a projectile for removal.
 * @pre Called inside a raylib drawing frame.
 */
bool projectile_move_and_draw(Ammo *ammo, double delta_time, void *context);

/**
 * @brief Decrease an enemy's remaining contact-damage cooldown.
 * @param enemy Enemy whose attack timer is updated.
 * @param delta_time Elapsed simulation time in seconds.
 * @param context Unused visitor context.
 * @return Always false; this update does not report a collision.
 * @note The timer may become negative because it is not clamped to zero.
 */
bool enemy_update_attack_cooldown(Enemy *enemy, double delta_time,
                                 void *context);

/**
 * @brief Apply contact damage when an enemy overlaps a ship and can attack.
 * @param enemy Enemy whose contact cooldown is reset when damage is applied.
 * @param spaceship Ship whose health may decrease.
 * @return True if the ship's resulting health is zero or less, even without a
 * hit.
 * @note This helper remains inactive in the main game loop.
 */
bool collision_spaceship(Enemy *enemy, Spaceship *spaceship);

#endif
