#include <assert.h>

#include "game_adapter.h"

static int callback_count;

#ifdef TEST_ORIGINAL
#define VISITOR_CONTEXT
#define IGNORE_CONTEXT
#else
#define VISITOR_CONTEXT , void *context
#define IGNORE_CONTEXT (void)context;
#endif

static bool count_enemy(Enemy *enemy, double delta_time VISITOR_CONTEXT) {
  IGNORE_CONTEXT
  (void)enemy;
  (void)delta_time;
  callback_count++;
  return true;
}

static bool count_projectile(Ammo *ammo, double delta_time VISITOR_CONTEXT) {
  IGNORE_CONTEXT
  (void)ammo;
  (void)delta_time;
  callback_count++;
  return false;
}

static void print_vector(const char *label, Vector2 vector) {
  printf("%s: %.9g %.9g\n", label, vector.x, vector.y);
}

static void release_enemy(Enemy *enemy) {
  free(enemy->velocity);
  free(enemy);
}

int main(void) {
  projectiles = calloc(1, sizeof(*projectiles));
  enemies = calloc(1, sizeof(*enemies));
  Initialize_Assets();
  Initialize_Agents();

  Vector2 zero = Clamp((Vector2){0, 0}, 400);
  assert(zero.x == 0 && zero.y == 0);
  print_vector("normalized diagonal", Clamp((Vector2){1, -1}, 400));

  for (int control = 0; control <= 31; control++) {
    spaceships[0]->position = (Vector2){400, 700};
    spaceships[0]->weapon->cd_remain = 0;
    Control_Spaceship(0, control, 0.125);
    printf("control %d: %.9g %.9g %.9g\n", control,
           spaceships[0]->position.x, spaceships[0]->position.y,
           spaceships[0]->weapon->cd_remain);
  }

  spaceships[0]->position = (Vector2){990, -10};
  Control_Spaceship(0, RIGHT | UP, 1);
  assert(spaceships[0]->position.x == 975);
  assert(spaceships[0]->position.y == 0);
  print_vector("clamped boundaries", spaceships[0]->position);

  Enemy *variants[] = {
    Aliencraft_Type_1(), Aliencraft_Type_2(), Aliencraft_Type_3()
  };
  for (int i = 0; i < 3; i++) {
    Enemy *enemy = variants[i];
    printf("variant %d: %s %d %.9g %.9g %.9g %.9g %.9g\n", i,
           enemy->name, enemy->points_worth, enemy->health, enemy->damage,
           enemy->velocity->x, enemy->acceleration, enemy->attack_cooldown);
    print_vector("enemy initial position", enemy->position);
    enemy->FlightPath(&enemy->position, 0.125, enemy->velocity,
                      enemy->acceleration);
    print_vector("enemy moved position", enemy->position);
    print_vector("enemy moved velocity", *enemy->velocity);
    release_enemy(enemy);
  }

  Ammo *blocked = ShootMissile(0.01, (Vector2){10, 20}, 2);
  assert(blocked == NULL);
  Ammo *missile = ShootMissile(0, (Vector2){10, 20}, 2);
  missile->Trajectory(&missile->position, 0.1, missile->velocity,
                      missile->acceleration);
  print_vector("missile moved position", missile->position);
  assert(missile->belongs_to == 2 && missile->damage == 10);
  free(missile->velocity);
  free(missile);

  /* Preserve the existing head-removal callback order, including its skip. */
  Enemy *first = Aliencraft_Type_1();
  Enemy *second = Aliencraft_Type_2();
  Enemy *third = Aliencraft_Type_3();
  first->position.y = WINDOW_HEIGHT + 1;
  Add_Enemy(first);
  Add_Enemy(second);
  Add_Enemy(third);
  callback_count = 0;
  assert(Iterate_Enemies(count_enemy, 0));
  assert(callback_count == 2 && enemies->head->enemy == second);
  printf("enemy removal callback count: %d\n", callback_count);

  Ammo *offscreen = ShootMissile(0, (Vector2){0, -11}, 0);
  Add_Projectile(offscreen);
  callback_count = 0;
  Iterate_Projectiles(count_projectile, 0);
  printf("projectile callback count: %d\n", callback_count);

  second->position = (Vector2){100, 100};
  third->position = (Vector2){500, 500};
  second->health = 10;
  Ammo *hit = ShootMissile(0, second->position, 1);
  assert(Detect_Projectile_Collisions(hit, 0));
  assert(second->dead && score[1] == 4);
  printf("destroyed enemy: %.9g %.9g %.9g %.9g %d\n", second->health,
         second->size_r, second->damage, second->attack_cooldown, score[1]);
  assert(!Detect_Projectile_Collisions(hit, 0));
  free(hit->velocity);
  free(hit);

  spaceships[1]->position = second->position;
  float health = spaceships[1]->health;
  Detect_Spaceship_Collision(second, 0, spaceships[1]);
  assert(spaceships[1]->health == health - 1);
  Detect_Spaceship_Collision(second, 0, spaceships[1]);
  assert(spaceships[1]->health == health - 1);
  printf("spaceship contact: %.9g %.9g\n", spaceships[1]->health,
         second->attack_cd_at);

  Enemy_Data_List enemy_data = {0};
  Projectile_Data_List projectile_data = {0};
  current_enemy_data_list = &enemy_data;
  current_projectile_data_list = &projectile_data;
  Add_Enemy_Data_Wrapper(second, 0);
  Ammo *captured = ShootMissile(0, (Vector2){12, 34}, 1);
  Add_Projectile_Data_Wrapper(captured, 0);
  free(captured->velocity);
  free(captured);

  Data_List snapshots = {0};
  Data data = {
    .keys_down = 17,
    .current_time_ms = 123,
    .frame_number = 4,
    .current_score = 9,
    .spaceship_data = {
      .position = {12, 34}, .health = 49, .size_radius = 25,
      .current_weapon_cd = 0.125
    },
    .enemy_data_list = enemy_data,
    .projectile_data_list = projectile_data
  };
  Add_To_Data(data, &snapshots);
  char *json = Iterate_Data(snapshots, Convert_Data_To_String);
  printf("snapshot: %s\n", json);
  free(json);
  Add_To_Data(data, &snapshots);
  json = Iterate_Data(snapshots, Convert_Data_To_String);
  printf("two snapshots: %s\n", json);
  free(json);
  free(snapshots.head->next);
  free(snapshots.head);
  free(enemy_data.head);
  free(projectile_data.head);

  char *empty_json = Iterate_Data((Data_List){0}, Convert_Data_To_String);
  assert(strcmp(empty_json, "[]") == 0);
  free(empty_json);
  json = Convert_Data_To_String((Data){0});
  printf("empty snapshot: %s\n", json);
  free(json);

  puts("Characterization assertions passed.");
  return 0;
}
