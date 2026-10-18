#include "data.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Define the snapshot format once for sizing and writing. */
static const char snapshot_format[] =
  "{\n"
  "\t\"key_strokes\": %d,\n"
  "\t\"timestamp\": %d,\n"
  "\t\"score\": %d,\n"
  "\t\"frame_number\": %d,\n"
  "\t\"spaceship\": {\n"
  "\t\t\"health\": %f,\n"
  "\t\t\"size\": %f,\n"
  "\t\t\"weapon_cd\": %f,\n"
  "\t\t\"position\": {\n"
  "\t\t\t\"x\": %f,\n"
  "\t\t\t\"y\": %f\n"
  "\t\t}\n"
  "\t},\n"
  "\t\"enemies\": [\n"
  "%s\n"
  "\t],\n"
  "\t\"projectiles\": [\n"
  "%s\n"
  "\t]\n"
  "},\n";

/**
 * @brief Allocate a precisely sized string using printf-style formatting.
 * @param format Valid printf format describing the variadic arguments.
 * @param ... Values matching the format's conversion specifiers.
 * @return A heap-owned formatted string that the caller must free.
 * @pre Formatting succeeds and the resulting allocation succeeds.
 */
static char *format_string(const char *format, ...) {
  va_list arguments;
  va_start(arguments, format);
  va_list sizing_arguments;
  va_copy(sizing_arguments, arguments);
  int length = vsnprintf(NULL, 0, format, sizing_arguments);
  va_end(sizing_arguments);
  char *text = malloc((size_t)length + 1);
  vsnprintf(text, (size_t)length + 1, format, arguments);
  va_end(arguments);
  return text;
}

/** @copydoc snapshot_string_append */
char *snapshot_string_append(const char *text, char *owned_string) {
  size_t capacity = strlen(text) + strlen(owned_string) + 1;
  char *expanded = realloc(owned_string, capacity);
  if (expanded == NULL) {
    return NULL;
  }
  strcat(expanded, text);
  return expanded;
}

/**
 * @brief Append an owned serialized fragment and release that fragment.
 * @param owned_string Heap destination that may move during concatenation.
 * @param serialized Heap-owned, null-terminated fragment consumed by this call.
 * @return The resized destination, or NULL when its reallocation fails.
 * @note On failure, the original destination allocation remains allocated.
 */
static char *append_serialized(char *owned_string, char *serialized) {
  char *result = snapshot_string_append(serialized, owned_string);
  free(serialized);
  return result;
}

/* Original serializers append ",\n" after every element. */
/**
 * @brief Remove the final two characters from a nonempty serialized fragment
 * list.
 * @param text Mutable, null-terminated concatenation expected to end in
 * comma-newline.
 * @note Strings of length two or less remain unchanged, preserving empty-list
 * output.
 */
static void remove_trailing_separator(char *text) {
  size_t length = strlen(text);
  if (length > 2) {
    text[length - 2] = '\0';
  }
}

/** @copydoc snapshot_enemy_append */
void snapshot_enemy_append(Enemy_Data data, Enemy_Data_List *list) {
  Enemy_Data_Node *node = calloc(1, sizeof(*node));
  node->enemy_data = data;
  if (list->head == NULL) {
    list->head = node;
  } else {
    list->tail->next = node;
  }
  list->tail = node;
}

/** @copydoc snapshot_capture_enemy */
bool snapshot_capture_enemy(Enemy *enemy, double delta_time, void *context) {
  (void)delta_time;
  Enemy_Data data = {
    .position = enemy->position,
    .health = enemy->health,
    .velocity = *enemy->velocity,
    .size_radius = enemy->radius,
    .name = enemy->name,
    .dead = enemy->dead
  };
  snapshot_enemy_append(data, context);
  return false;
}

/** @copydoc snapshot_serialize_enemies */
char *snapshot_serialize_enemies(Enemy_Data_List list,
                             char *(*serialize)(Enemy_Data)) {
  char *text = calloc(1, 1);
  for (Enemy_Data_Node *node = list.head; node != NULL; node = node->next) {
    text = append_serialized(text, serialize(node->enemy_data));
  }
  return text;
}

/** @copydoc snapshot_projectile_append */
void snapshot_projectile_append(Projectile_Data data,
                                Projectile_Data_List *list) {
  Projectile_Data_Node *node = calloc(1, sizeof(*node));
  node->projectile_data = data;
  if (list->head == NULL) {
    list->head = node;
  } else {
    list->tail->next = node;
  }
  list->tail = node;
}

/** @copydoc snapshot_capture_projectile */
bool snapshot_capture_projectile(Ammo *ammo, double delta_time,
                                 void *context) {
  (void)delta_time;
  Projectile_Data data = {
    .position = ammo->position,
    .damage = ammo->damage,
    .velocity = *ammo->velocity,
    .size_radius = ammo->radius
  };
  snapshot_projectile_append(data, context);
  return false;
}

/** @copydoc snapshot_serialize_projectiles */
char *snapshot_serialize_projectiles(Projectile_Data_List list,
                                  char *(*serialize)(Projectile_Data)) {
  char *text = calloc(1, 1);
  for (Projectile_Data_Node *node = list.head; node != NULL;
       node = node->next) {
    text = append_serialized(text, serialize(node->projectile_data));
  }
  return text;
}

/** @copydoc snapshot_append */
void snapshot_append(Data data, Data_List *list) {
  Data_Node *node = calloc(1, sizeof(*node));
  node->data = data;
  if (list->head == NULL) {
    list->head = node;
  } else {
    list->tail->next = node;
  }
  list->tail = node;
}

/** @copydoc snapshot_serialize_list */
char *snapshot_serialize_list(Data_List list, char *(*serialize)(Data)) {
  char *elements = calloc(1, 1);
  for (Data_Node *node = list.head; node != NULL; node = node->next) {
    elements = append_serialized(elements, serialize(node->data));
  }
  remove_trailing_separator(elements);
  char *json = format_string("[%s]", elements);
  free(elements);
  return json;
}

/** @copydoc snapshot_serialize_enemy */
char *snapshot_serialize_enemy(Enemy_Data data) {
  /* Retain the original capacity and truncation behavior. */
  char *text = malloc(300);
  snprintf(text, 300,
    "\t\t{\n"
    "\t\t\t\"health\": %f,\n"
    "\t\t\t\"size\": %f,\n"
    "\t\t\t\"dead\": %d,\n"
    "\t\t\t\"name\": \"%s\",\n"
    "\t\t\t\"velocity\": {\n"
    "\t\t\t\t\"x\": %f,\n"
    "\t\t\t\t\"y\": %f\n"
    "\t\t\t},\n"
    "\t\t\t\"position\": {\n"
    "\t\t\t\t\"x\": %f,\n"
    "\t\t\t\t\"y\": %f\n"
    "\t\t\t}\n"
    "\t\t},\n",
    data.health, data.size_radius, data.dead, data.name,
    data.velocity.x, data.velocity.y, data.position.x, data.position.y);
  return text;
}

/** @copydoc snapshot_serialize_projectile */
char *snapshot_serialize_projectile(Projectile_Data data) {
  char *text = malloc(200);
  snprintf(text, 200,
    "\t\t{\n"
    "\t\t\t\"damage\": %f,\n"
    "\t\t\t\"size\": %f,\n"
    "\t\t\t\"velocity\": {\n"
    "\t\t\t\t\"x\": %f,\n"
    "\t\t\t\t\"y\": %f\n"
    "\t\t\t},\n"
    "\t\t\t\"position\": {\n"
    "\t\t\t\t\"x\": %f,\n"
    "\t\t\t\t\"y\": %f\n"
    "\t\t\t}\n"
    "\t\t},\n",
    data.damage, data.size_radius, data.velocity.x, data.velocity.y,
    data.position.x, data.position.y);
  return text;
}

/** @copydoc snapshot_serialize */
char *snapshot_serialize(Data data) {
  char *enemies_json = snapshot_serialize_enemies(data.enemy_data_list,
                                             snapshot_serialize_enemy);
  char *projectiles_json = snapshot_serialize_projectiles(
    data.projectile_data_list, snapshot_serialize_projectile);
  remove_trailing_separator(enemies_json);
  remove_trailing_separator(projectiles_json);

  Spaceship_Data ship = data.spaceship_data;
  char *text = format_string(snapshot_format,
    data.keys_down, data.current_time_ms, data.current_score,
    data.frame_number, ship.health, ship.size_radius, ship.current_weapon_cd,
    ship.position.x, ship.position.y, enemies_json, projectiles_json);
  free(projectiles_json);
  free(enemies_json);
  return text;
}
