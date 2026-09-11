#include <raylib.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "Headers/global_declarations.h"
#include "Headers/macros.h"
#include "Headers/objects.h"
#include "Headers/data.h"
#include "Headers/object_collections.h"
#include "Headers/detections.h"


//If you move left + up, it will move faster versus moving just left or up. This normalize it.
Vector2 Clamp(Vector2 delta, float speed){

    float mag = sqrt(delta.x * delta.x + delta.y * delta.y);

    if(mag == 0){
        return delta;
    }

    //delta.x/mag , delta.y/mag === gives the unit vector
    delta.x = speed * delta.x/mag; //cosine * speed, if only moving up or down, this will be delta.x will be 0
    delta.y = speed * delta.y/mag; //sine * speed, if only moving left or right, delta.y will be 0

    return delta;
}


int main(){

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Test");

    data_list = calloc(1, sizeof(Data_List));

    enemy_img_1 = LoadImage("Pictures/aliencraft1.png");
    ImageResize(&enemy_img_1, ENEMY_RADIUS * 2, ENEMY_RADIUS * 2);
    enemy_img_2 = LoadImage("Pictures/aliencraft2.png");
    ImageResize(&enemy_img_2, ENEMY_RADIUS * 2, ENEMY_RADIUS * 2);
    enemy_img_3 = LoadImage("Pictures/aliencraft3.png");
    ImageResize(&enemy_img_3, ENEMY_RADIUS * 2, ENEMY_RADIUS * 2);

    enemy_model_1 = LoadTextureFromImage(enemy_img_1);
    enemy_model_2 = LoadTextureFromImage(enemy_img_2);
    enemy_model_3 = LoadTextureFromImage(enemy_img_3);

    ammo_image = LoadImage("Pictures/ammo1.png");
    ImageResize(&ammo_image, MISSILE_RADIUS * 2, MISSILE_RADIUS * 3);

    ammo_model = LoadTextureFromImage(ammo_image);

    explosion_img = LoadImage("Pictures/explosion.png");
    ImageResize(&explosion_img, EXPLOSION_RADIUS * 2, EXPLOSION_RADIUS * 2);

    explosion_model = LoadTextureFromImage(explosion_img);

    int enemy_variant_count = 3;

    //Contains 3 different function pointers that each create a different a different type of enemy
    Enemy* (*enemy_variants[3])() = {
        Aliencraft_Type_1,
        Aliencraft_Type_2,
        Aliencraft_Type_3
    };

    projectiles = calloc(1, sizeof(Projectile_Collections));
    enemies = calloc(1, sizeof(Enemy_Collections));

    SetTargetFPS(120);

    //The enemy spawn_timer
    float spawn_timer = ENEMY_SPAWN_TIMER;

    int accumulated_time = 0;

    spaceship = Create_Spaceship();

    spaceship->position = (Vector2){400, 700};

    double last_time = GetTime();

    Font font = LoadFont("Fonts/NotoSans-VariableFont.ttf");

    while (!WindowShouldClose()) {

        double this_time = GetTime();

        double delta_time = this_time - last_time;

        spawn_timer -= delta_time;

        Vector2 delta = {0,0};

        int keys_down = 0;

        if(IsKeyDown(KEY_A)){
            delta.x = -1;
            keys_down |= LEFT;

        } 
        else if(IsKeyDown(KEY_D)){
            delta.x = 1;
            keys_down |= RIGHT;
        }

        if(IsKeyDown(KEY_W)){
            delta.y = -1;
            keys_down |= UP;
        }
        else if(IsKeyDown(KEY_S)){
            delta.y = 1;
            keys_down |= DOWN;
        }


        delta = Clamp(delta, spaceship->speed);

        spaceship->position.x += delta.x * delta_time;
        spaceship->position.y += delta.y * delta_time;

        if(spaceship->position.x > WINDOW_WIDTH - SPACESHIP_RADIUS){
            spaceship->position.x = WINDOW_WIDTH - SPACESHIP_RADIUS;
        } else if(spaceship->position.x < 0){
            spaceship->position.x = 0;
        }
        
        if(spaceship->position.y > WINDOW_HEIGHT - SPACESHIP_RADIUS){
            spaceship->position.y = WINDOW_HEIGHT - SPACESHIP_RADIUS;
        } else if(spaceship->position.y < 0){
            spaceship->position.y = 0;
        }

        if(IsKeyPressed(KEY_O)){

            keys_down |= SHOOT;

            Ammo *ammo = spaceship->weapon->Shoot(spaceship->weapon->cd_remain, spaceship->position);

        
            if(ammo){
                spaceship->weapon->cd_remain = spaceship->weapon->cooldown;
                Add_Projectile(ammo);
            }

        }

        spaceship->weapon->cd_remain -= delta_time;

        last_time = this_time;

        //The enemy spawn spawn_timer
        if(spawn_timer <= 0.0f){

            int choose = GetRandomValue(0,2);

            Enemy *enemy = enemy_variants[choose]();

            Add_Enemy(enemy);

            spawn_timer = ENEMY_SPAWN_TIMER;
        }



        BeginDrawing();

            ClearBackground(BLACK);

            DrawTexture(spaceship->model, spaceship->position.x, spaceship->position.y, WHITE);

            Iterate_Projectiles(Detect_Projectile_Collisions, delta_time);


            Iterate_Enemies(Move_Enemy, delta_time);
            Iterate_Enemies(Update_Enemy_Attack_Cooldown, delta_time);
            Iterate_Projectiles(Move_Projectile, delta_time);


            char score_text[100];
            char health_text[25];
            char time_elapsed[25];

            snprintf(score_text, 100, "Total Score: %d", score);

            DrawTextEx(font, score_text, (Vector2){50,50}, 24, 1, WHITE);

            snprintf(health_text, 25, "Health: %f", spaceship->health);

            DrawTextEx(font, health_text, (Vector2){WINDOW_WIDTH - 300, 50}, 24, 1, WHITE);

            snprintf(time_elapsed, 25, "Time: %d", accumulated_time);

            DrawTextEx(font, time_elapsed, (Vector2){WINDOW_WIDTH/2, 50}, 24, 1, WHITE);

        EndDrawing();


        frame_count++;

        //Collects data
        accumulated_time += floor(delta_time * 1000);

        if(frame_count % 4 != 0){
            continue;
        }

        Enemy_Data_List *enemies_data = calloc(1, sizeof(Enemy_Data_List));
        Projectile_Data_List *projectiles_data = calloc(1, sizeof(Projectile_Data_List));


        current_enemy_data_list = enemies_data;
   
        current_projectile_data_list = projectiles_data;

        Iterate_Enemies(Add_Enemy_Data_Wrapper, delta_time);
        Iterate_Projectiles(Add_Projectile_Data_Wrapper, delta_time);

        Spaceship_Data new_spaceship_data_entry = {
            .position = spaceship->position,
            .health = spaceship->health,
            .size_radius = spaceship->size_r,
            .current_weapon_cd = spaceship->weapon->cd_remain
        };

        Data data_entry = {
            .keys_down = keys_down,
            .current_time_ms = accumulated_time,
            .spaceship_data = new_spaceship_data_entry,
            .enemy_data_list = *enemies_data,
            .projectile_data_list = *projectiles_data,
            .frame_number = frame_count,
            .current_score = score
        };

        Add_To_Data(data_entry, data_list);

        //Check see if spaceship hit onto any enemy, if so, then game over
        if(Iterate_Enemies(Detect_Spaceship_Collision, delta_time)){
            break;
        }

        if(accumulated_time > 180000){
            break;
        }
    }

    char* json_str = Iterate_Data(*data_list, Convert_Data_To_String);

    FILE *file = fopen("Game_Data/game_snapshots.json", "w");

    if (file == NULL) {
        printf("Failed to open file.\n");
        return 1;
    }

    fputs(json_str, file);

    fclose(file);

    UnloadImage(enemy_img_1);
    UnloadImage(enemy_img_2);
    UnloadImage(enemy_img_3);
    UnloadImage(ammo_image);
    UnloadImage(explosion_img);

    CloseWindow();

    return 1;
}