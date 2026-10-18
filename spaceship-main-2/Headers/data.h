#ifndef SPACESHIP_DATA_H
#define SPACESHIP_DATA_H

#include "objects.h"

/* Snapshot values copy entity state; linked nodes own only those values. */
struct Enemy_Data {
  Vector2 position;
  float health;
  Vector2 velocity;
  float size_radius;
  const char *name;
  bool dead;
};

struct Enemy_Data_Node {
  Enemy_Data_Node *next;
  Enemy_Data enemy_data;
};

struct Enemy_Data_List {
  Enemy_Data_Node *head;
  Enemy_Data_Node *tail;
};

struct Projectile_Data {
  Vector2 position;
  float damage;
  Vector2 velocity;
  float size_radius;
};

struct Projectile_Data_Node {
  Projectile_Data_Node *next;
  Projectile_Data projectile_data;
};

struct Projectile_Data_List {
  Projectile_Data_Node *head;
  Projectile_Data_Node *tail;
};

struct Spaceship_Data {
  Vector2 position;
  float health;
  float size_radius;
  float current_weapon_cd;
};

struct Data {
  int keys_down;
  int current_time_ms;
  int frame_number;
  int current_score;
  Spaceship_Data spaceship_data;
  Enemy_Data_List enemy_data_list;
  Projectile_Data_List projectile_data_list;
};

struct Data_Node {
  Data_Node *next;
  Data data;
};

struct Data_List {
  Data_Node *head;
  Data_Node *tail;
};

/**
 * @brief Append an enemy snapshot value to a snapshot list.
 * @param data Value to copy; the enemy name pointer remains borrowed.
 * @param list Initialized destination list that owns the new node.
 * @pre The node allocation succeeds and borrowed names outlive serialization.
 */
void snapshot_enemy_append(Enemy_Data data, Enemy_Data_List *list);

/**
 * @brief Append a copied projectile snapshot to a snapshot list.
 * @param data Snapshot value copied into a newly allocated node.
 * @param list Initialized destination list that owns the new node.
 * @pre The node allocation succeeds.
 */
void snapshot_projectile_append(Projectile_Data data,
                                Projectile_Data_List *list);

/**
 * @brief Append a frame snapshot using a shallow copy of its nested list
 * handles.
 * @param data Frame value to copy; nested snapshot nodes remain caller-managed.
 * @param list Initialized destination list that owns the new frame node.
 * @pre Allocation succeeds and nested lists remain valid through serialization.
 */
void snapshot_append(Data data, Data_List *list);

/* Compatible collection visitors; context points to the destination list. */
/**
 * @brief Copy an enemy's current state into a destination snapshot list.
 * @param enemy Enemy whose velocity and scalar state are read.
 * @param delta_time Unused elapsed seconds, retained for visitor compatibility.
 * @param context Destination Enemy_Data_List pointer.
 * @return Always false, so capture does not report a collision.
 * @note The snapshot copies velocity and position but borrows the name pointer.
 */
bool snapshot_capture_enemy(Enemy *enemy, double delta_time, void *context);

/**
 * @brief Copy a projectile's current state into a destination snapshot list.
 * @param ammo Projectile whose velocity, position, and scalar fields are read.
 * @param delta_time Unused elapsed seconds, retained for visitor compatibility.
 * @param context Destination Projectile_Data_List pointer.
 * @return Always false, so capture does not request projectile removal.
 */
bool snapshot_capture_projectile(Ammo *ammo, double delta_time,
                                 void *context);

/* Every serializer returns an allocated string owned by the caller. */
/**
 * @brief Resize a heap string and append another null-terminated string.
 * @param text Borrowed source text that does not overlap the destination
 * buffer.
 * @param owned_string Heap-owned destination, possibly relocated by realloc.
 * @return The resized string, or NULL if resizing fails.
 * @note On success use the returned pointer; the previous pointer may be
 * invalid.
 * @note On failure the original allocation is still owned by the caller.
 */
char *snapshot_string_append(const char *text, char *owned_string);

/**
 * @brief Concatenate serialized enemy fragments without adding array brackets.
 * @param list Borrowed list whose nodes are visited in insertion order.
 * @param serialize Callback returning an allocated, null-terminated fragment.
 * @return An allocated concatenation, or an empty string for an empty list.
 * @note Callback results are freed; separators supplied by the callback remain.
 * @note The caller frees the returned string; allocation failures are not
 * handled.
 */
char *snapshot_serialize_enemies(Enemy_Data_List list,
                             char *(*serialize)(Enemy_Data));

/**
 * @brief Concatenate serialized projectile fragments without array brackets.
 * @param list Borrowed list whose nodes are visited in insertion order.
 * @param serialize Callback returning an allocated, null-terminated fragment.
 * @return An allocated concatenation, or an empty string for an empty list.
 * @note Callback results are freed; separators supplied by the callback remain.
 * @note The caller frees the returned string; allocation failures are not
 * handled.
 */
char *snapshot_serialize_projectiles(Projectile_Data_List list,
                                  char *(*serialize)(Projectile_Data));

/**
 * @brief Serialize frame snapshots into a JSON array using the supplied
 * callback.
 * @param list Borrowed frame list to serialize in insertion order.
 * @param serialize Callback returning allocated fragments ending with
 * comma-newline.
 * @return An allocated array string, including [] for an empty list; caller
 * frees it.
 * @note Callback results are freed after appending; allocation failure is
 * unhandled.
 */
char *snapshot_serialize_list(Data_List list, char *(*serialize)(Data));

/**
 * @brief Format one enemy snapshot as the original indented JSON fragment.
 * @param data Snapshot value containing a valid borrowed enemy name.
 * @return A heap string ending with comma-newline; the caller must free it.
 * @note The 300-byte capacity may truncate output; names are not JSON-escaped.
 * @pre String allocation succeeds.
 */
char *snapshot_serialize_enemy(Enemy_Data data);

/**
 * @brief Format one projectile snapshot as the original indented JSON fragment.
 * @param data Snapshot value to format.
 * @return A heap string ending with comma-newline; the caller must free it.
 * @note The original 200-byte capacity, including the terminator, may truncate
 * output.
 * @pre String allocation succeeds.
 */
char *snapshot_serialize_projectile(Projectile_Data data);

/**
 * @brief Format one frame with spaceship, enemy, and projectile snapshot data.
 * @param data Frame snapshot whose nested lists remain valid during formatting.
 * @return An allocated object fragment ending with comma-newline; caller frees
 * it.
 * @note Does not write a file or free snapshot nodes; allocation failure is
 * unhandled.
 */
char *snapshot_serialize(Data data);

#endif
