#include "objects.h"

#include <math.h>
#include <stdlib.h>

/* Each variant is data; creation and motion stay shared. */
typedef struct Enemy_Definition {
  const char *name;
  int points;
  float health;
  float damage;
  float horizontal_speed;
  float acceleration;
  float attack_cooldown;
} Enemy_Definition;

static const Enemy_Definition enemy_definitions[] = {
  {
    .name = ENEMY_TYPE_1,
    .points = 2,
    .health = 10,
    .damage = 5,
    .horizontal_speed = 100,
    .acceleration = 75,
    .attack_cooldown = ENEMY_TYPE_1_ATTACK_CD
  },
  {
    .name = ENEMY_TYPE_2,
    .points = 4,
    .health = 15,
    .damage = 7,
    .horizontal_speed = 150,
    .acceleration = 50,
    .attack_cooldown = ENEMY_TYPE_2_ATTACK_CD
  },
  {
    .name = ENEMY_TYPE_3,
    .points = 8,
    .health = 25,
    .damage = 10,
    .horizontal_speed = 100,
    .acceleration = 20,
    .attack_cooldown = ENEMY_TYPE_3_ATTACK_CD
  }
};

/** @copydoc motion_accelerate_up */
void motion_accelerate_up(Vector2 *position, float delta_time,
                         Vector2 *velocity, float acceleration) {
  velocity->y += acceleration * delta_time;
  position->y -= velocity->y * delta_time;
}

/** @copydoc motion_accelerate_zigzag */
void motion_accelerate_zigzag(Vector2 *position, float delta_time,
                             Vector2 *velocity, float acceleration) {
  velocity->y += acceleration * delta_time;
  position->y += velocity->y * delta_time;

  int horizontal_band = (int)floor(position->x) % 350;
  if (horizontal_band < 10 && horizontal_band > 0) {
    velocity->x *= -1;
  }
  position->x += velocity->x * delta_time;
}

/** @copydoc enemy_create */
Enemy *enemy_create(Enemy_Variant variant, Texture model) {
  const Enemy_Definition *definition = &enemy_definitions[variant];
  Enemy *enemy = calloc(1, sizeof(*enemy));
  enemy->velocity = calloc(1, sizeof(*enemy->velocity));
  enemy->velocity->y = 10.0f;
  enemy->velocity->x = definition->horizontal_speed;
  enemy->position = (Vector2){GetRandomValue(0, 800), -ENEMY_RADIUS};
  enemy->points_worth = definition->points;
  enemy->health = definition->health;
  enemy->damage = definition->damage;
  enemy->radius = ENEMY_RADIUS;
  enemy->acceleration = definition->acceleration;
  enemy->flight_path = motion_accelerate_zigzag;
  enemy->name = definition->name;
  enemy->attack_cooldown = definition->attack_cooldown;
  enemy->model = model;
  return enemy;
}

/** @copydoc projectile_create */
Ammo *projectile_create(float cooldown, Vector2 position, int owner,
                        Texture model) {
  if (cooldown > 0) {
    return NULL;
  }

  Ammo *ammo = calloc(1, sizeof(*ammo));
  ammo->velocity = calloc(1, sizeof(*ammo->velocity));
  ammo->velocity->y = 30.0f;
  ammo->position = position;
  ammo->damage = 10.0f;
  ammo->model = model;
  ammo->damage_area_radius = EXPLOSION_RADIUS;
  ammo->trajectory = motion_accelerate_up;
  ammo->radius = MISSILE_RADIUS;
  ammo->acceleration = 300.0f;
  ammo->owner = owner;
  return ammo;
}

/** @copydoc spaceship_create */
Spaceship *spaceship_create(Texture model, Texture projectile_model) {
  Spaceship *spaceship = calloc(1, sizeof(*spaceship));
  Weapon *weapon = calloc(1, sizeof(*weapon));
  weapon->cooldown = 0.25f;
  weapon->shoot = projectile_create;
  weapon->projectile_model = projectile_model;
  spaceship->health = SPACESHIP_HEALTH;
  spaceship->speed = SPACESHIP_SPEED;
  spaceship->weapon = weapon;
  spaceship->radius = SPACESHIP_RADIUS;

  spaceship->model = model;
  return spaceship;
}

/** @copydoc enemy_destroy */
void enemy_destroy(Enemy *enemy) {
  free(enemy->velocity);
  free(enemy);
}

/** @copydoc projectile_destroy */
void projectile_destroy(Ammo *ammo) {
  free(ammo->velocity);
  free(ammo);
}
