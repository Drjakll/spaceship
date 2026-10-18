#ifndef SPACESHIP_TEST_GAME_ADAPTER_H
#define SPACESHIP_TEST_GAME_ADAPTER_H

/* Fixtures stay identical across the original and explicit-state APIs. */
#ifdef TEST_ORIGINAL
#define main Original_Game_Main
#include "../.work/baseline/init.c"
#undef main
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "data.h"

static Game_State test_game;
static Enemy_Data_List *current_enemy_data_list;
static Projectile_Data_List *current_projectile_data_list;

#define projectiles test_game.projectiles
#define enemies test_game.enemies
#define spaceships test_game.spaceships
#define score test_game.scores
#define cd_remain cooldown_remaining
#define size_r radius
#define belongs_to owner
#define FlightPath flight_path
#define Trajectory trajectory
#define attack_cd_at attack_cooldown_remaining
#define Clamp movement_normalize
#define Add_To_Data snapshot_append
#define Iterate_Data snapshot_serialize_list
#define Convert_Data_To_String snapshot_serialize

static void Initialize_Assets(void) {
  assets_load(&test_game.assets);
}

static void Initialize_Agents(void) {
  game_initialize_agents(&test_game);
}

static void Control_Spaceship(int agent, int actions, double delta_time) {
  game_control_spaceship(&test_game, agent, actions, delta_time);
}

static Enemy *Aliencraft_Type_1(void) {
  return enemy_create(ENEMY_VARIANT_1, test_game.assets.enemy_models[0]);
}

static Enemy *Aliencraft_Type_2(void) {
  return enemy_create(ENEMY_VARIANT_2, test_game.assets.enemy_models[1]);
}

static Enemy *Aliencraft_Type_3(void) {
  return enemy_create(ENEMY_VARIANT_3, test_game.assets.enemy_models[2]);
}

static Ammo *ShootMissile(float cooldown, Vector2 position, int owner) {
  return projectile_create(cooldown, position, owner,
                            test_game.assets.ammo_model);
}

static void Add_Enemy(Enemy *enemy) {
  game_add_enemy(&test_game, enemy);
}

static void Add_Projectile(Ammo *ammo) {
  game_add_projectile(&test_game, ammo);
}

static bool Iterate_Enemies(Enemy_Visitor visitor, double delta_time) {
  return enemy_collection_visit(enemies, visitor, delta_time, NULL);
}

static bool Iterate_Projectiles(Projectile_Visitor visitor,
                                double delta_time) {
  return projectile_collection_visit(projectiles, visitor, delta_time, NULL);
}

static bool Detect_Projectile_Collisions(Ammo *ammo, double delta_time) {
  Collision_Context collision = {
    enemies, score, test_game.assets.explosion_model
  };
  return collision_projectile(ammo, delta_time, &collision);
}

static bool Detect_Spaceship_Collision(Enemy *enemy, double delta_time,
                                      Spaceship *ship) {
  (void)delta_time;
  return collision_spaceship(enemy, ship);
}

static bool Add_Enemy_Data_Wrapper(Enemy *enemy, double delta_time) {
  return snapshot_capture_enemy(enemy, delta_time, current_enemy_data_list);
}

static bool Add_Projectile_Data_Wrapper(Ammo *ammo, double delta_time) {
  return snapshot_capture_projectile(ammo, delta_time,
                                     current_projectile_data_list);
}
#endif

#endif
