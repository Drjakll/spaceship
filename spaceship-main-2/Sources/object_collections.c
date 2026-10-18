#include "object_collections.h"

#include <stdlib.h>

/** @copydoc enemy_collection_insert */
void enemy_collection_insert(Enemy_Collections *collection, Enemy *enemy) {
  Enemy_Node *node = calloc(1, sizeof(*node));
  node->enemy = enemy;
  if (collection->head == NULL) {
    collection->head = node;
  } else {
    collection->tail->next = node;
  }
  collection->tail = node;
}

/**
 * @brief Unlink an enemy node and release its owned enemy and velocity.
 * @param collection Collection containing the node to remove.
 * @param previous Immediate predecessor, or NULL when removing the head.
 * @param node Valid node belonging to the collection.
 * @note Updates the head or predecessor link and repairs the tail when needed.
 */
static void remove_enemy(Enemy_Collections *collection, Enemy_Node *previous,
                         Enemy_Node *node) {
  if (previous == NULL) {
    collection->head = node->next;
  } else {
    previous->next = node->next;
  }
  if (collection->tail == node) {
    collection->tail = previous;
  }
  enemy_destroy(node->enemy);
  free(node);
}

/** @copydoc enemy_collection_visit */
bool enemy_collection_visit(Enemy_Collections *collection,
                            Enemy_Visitor visitor,
                            double delta_time, void *context) {
  Enemy_Node *head = collection->head;
  if (head == NULL) {
    return false;
  }

  bool collided = visitor(head->enemy, delta_time, context);
  if (head->enemy->position.y > WINDOW_HEIGHT) {
    remove_enemy(collection, NULL, head);
  }

  /* Preserve the original traversal: a promoted head waits until next pass. */
  Enemy_Node *previous = collection->head;
  while (previous != NULL && previous->next != NULL) {
    Enemy_Node *node = previous->next;
    if (visitor(node->enemy, delta_time, context)) {
      collided = true;
    }
    if (node->enemy->position.y > WINDOW_HEIGHT) {
      remove_enemy(collection, previous, node);
    } else {
      previous = node;
    }
  }
  return collided;
}

/** @copydoc projectile_collection_insert */
void projectile_collection_insert(Projectile_Collections *collection,
                                  Ammo *ammo) {
  Projectile_Node *node = calloc(1, sizeof(*node));
  node->ammo = ammo;
  if (collection->head == NULL) {
    collection->head = node;
  } else {
    collection->tail->next = node;
  }
  collection->tail = node;
}

/**
 * @brief Unlink a projectile node and release its projectile and velocity.
 * @param collection Collection containing the node to remove.
 * @param previous Immediate predecessor, or NULL when removing the head.
 * @param node Valid node belonging to the collection.
 * @note Updates the head or predecessor link and repairs the tail when needed.
 */
static void remove_projectile(Projectile_Collections *collection,
                              Projectile_Node *previous,
                              Projectile_Node *node) {
  if (previous == NULL) {
    collection->head = node->next;
  } else {
    previous->next = node->next;
  }
  if (collection->tail == node) {
    collection->tail = previous;
  }
  projectile_destroy(node->ammo);
  free(node);
}

/** @copydoc projectile_collection_visit */
bool projectile_collection_visit(Projectile_Collections *collection,
                                 Projectile_Visitor visitor,
                                 double delta_time, void *context) {
  Projectile_Node *head = collection->head;
  if (head == NULL) {
    return false;
  }

  bool collided = visitor(head->ammo, delta_time, context);
  if (collided || head->ammo->position.y < -MISSILE_RADIUS) {
    remove_projectile(collection, NULL, head);
  }

  /* The promoted head is intentionally deferred, as in the original game. */
  Projectile_Node *previous = collection->head;
  while (previous != NULL && previous->next != NULL) {
    Projectile_Node *node = previous->next;
    bool node_collided = visitor(node->ammo, delta_time, context);
    collided |= node_collided;
    if (node_collided || node->ammo->position.y < -MISSILE_RADIUS) {
      remove_projectile(collection, previous, node);
    } else {
      previous = node;
    }
  }
  return collided;
}
