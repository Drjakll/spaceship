#ifndef SPACESHIP_OBJECT_COLLECTIONS_H
#define SPACESHIP_OBJECT_COLLECTIONS_H

#include "objects.h"

typedef bool (*Enemy_Visitor)(Enemy *, double, void *);
typedef bool (*Projectile_Visitor)(Ammo *, double, void *);

struct Enemy_Node {
  Enemy_Node *next;
  Enemy *enemy;
};

struct Projectile_Node {
  Projectile_Node *next;
  Ammo *ammo;
};

struct Enemy_Collections {
  Enemy_Node *head;
  Enemy_Node *tail;
};

struct Projectile_Collections {
  Projectile_Node *head;
  Projectile_Node *tail;
};

/* Insertion transfers entity ownership to the collection. */
/**
 * @brief Append an enemy while maintaining the collection's head and tail
 * links.
 * @param collection Initialized destination collection.
 * @param enemy Non-NULL enemy whose ownership transfers to the collection.
 * @pre The new collection-node allocation succeeds.
 */
void enemy_collection_insert(Enemy_Collections *collection, Enemy *enemy);

/**
 * @brief Append a projectile to the collection's tail.
 * @param collection Initialized destination collection.
 * @param ammo Non-NULL projectile whose ownership transfers to the collection.
 * @pre The new collection-node allocation succeeds.
 */
void projectile_collection_insert(Projectile_Collections *collection,
                                  Ammo *ammo);

/* Visitors report collisions; traversal also removes expired entities. */
/**
 * @brief Visit enemies in list order and remove enemies below the window.
 * @param collection Owned collection whose links and entities may be updated.
 * @param visitor Callback receiving an enemy, elapsed seconds, and context.
 * @param delta_time Elapsed time in seconds forwarded unchanged to the
 * callback.
 * @param context Borrowed callback data; may be NULL if the callback allows it.
 * @return True if any invoked callback returns true; false for an empty list.
 * @pre The visitor does not free entities or change this collection's links.
 * @note If the head is removed, its successor waits until the next visit pass.
 * @note Callback results do not remove enemies; screen position controls
 * removal.
 */
bool enemy_collection_visit(Enemy_Collections *collection,
                            Enemy_Visitor visitor,
                            double delta_time, void *context);

/**
 * @brief Visit projectiles and remove collided missiles or missiles above
 * screen.
 * @param collection Owned collection whose links and projectiles may be
 * updated.
 * @param visitor Callback whose true result removes the current projectile.
 * @param delta_time Elapsed time in seconds forwarded unchanged to the
 * callback.
 * @param context Borrowed callback data; may be NULL if the callback allows it.
 * @return True if any invoked callback returns true; false for an empty list.
 * @pre The visitor does not free entities or change this collection's links.
 * @note If the head is removed, its successor waits until the next visit pass.
 */
bool projectile_collection_visit(Projectile_Collections *collection,
                                 Projectile_Visitor visitor,
                                 double delta_time, void *context);

#endif
