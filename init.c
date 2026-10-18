#include <raylib.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <cjson/cJSON.h>

#include "Headers/macros.h"
#include "Headers/global_declarations.h"
#include "Headers/ml_utilities.h"
#include "Headers/self_control.h"
#include "Headers/objects.h"
#include "Headers/object_collections.h"
#include "Headers/data.h"
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

void Initialize_Assets(){

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
}

void Initialize_Agents(){

    for(int i = 0; i < NUM_OF_AGENTS; i++){
        spaceships[i] = Create_Spaceship();

        spaceships[i]->snapshots = calloc(1, sizeof *spaceships[i]->snapshots);

        spaceships[i]->position = (Vector2){400 + 50*i, 700};
    }

}

void Control_Spaceship(int i, int control, double delta_time ){

    Spaceship *spaceship = spaceships[i];

    Vector2 delta = {0,0};

    if((control & LEFT) == LEFT){
        delta.x += -1;
    }

    if((control & RIGHT) == RIGHT){
        delta.x += 1;
    }

    if((control & UP) == UP){
        delta.y += -1;
    }

    if((control & DOWN) == DOWN){
        delta.y += 1;
    }

    delta = Clamp(delta, spaceship->speed);

    spaceship->position.x += delta.x * delta_time;
    spaceship->position.y += delta.y * delta_time;

    if(spaceship->position.x > WINDOW_WIDTH - 2 * SPACESHIP_RADIUS){
        spaceship->position.x = WINDOW_WIDTH - 2 * SPACESHIP_RADIUS;
    } else if(spaceship->position.x < 0){
        spaceship->position.x = 0;
    }
    
    if(spaceship->position.y > WINDOW_HEIGHT - 2 * SPACESHIP_RADIUS){
        spaceship->position.y = WINDOW_HEIGHT - 2 * SPACESHIP_RADIUS;
    } else if(spaceship->position.y < 0){
        spaceship->position.y = 0;
    }

    spaceship->weapon->cd_remain -= delta_time;

    if((control & SHOOT) == SHOOT){

        Ammo *ammo = spaceship->weapon->Shoot(spaceship->weapon->cd_remain, spaceship->position, i);

        if(ammo){
            spaceship->weapon->cd_remain = spaceship->weapon->cooldown;
            Add_Projectile(ammo);
        }
    }
    
}

bool Help_Ally(int self_index){


    for(int i = 1; i < NUM_OF_AGENTS; i++){

        int new_index = (self_index + i) % NUM_OF_AGENTS;

        if(best_target_probability[new_index] != NULL){
            best_target_probability[self_index] = best_target_probability[new_index];
            return true;
        }
    }
    
    return false;
}

int main(){

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Spaceship");

    distance_size = (int)sqrt(pow(WINDOW_WIDTH/MAP_FACTOR,2) + pow(WINDOW_HEIGHT/MAP_FACTOR, 2));

    //data_list = calloc(1, sizeof(Data_List));

    Initialize_Assets();

    SetTargetFPS(120);

    int enemy_variant_count = 3;

    //Contains 3 different function pointers that each create a different a different type of enemy
    Enemy* (*enemy_variants[3])() = {
        Aliencraft_Type_1,
        Aliencraft_Type_2,
        Aliencraft_Type_3
    };

    projectiles = calloc(1, sizeof(Projectile_Collections));
    enemy_collections = calloc(1, sizeof(Enemy_Collections));

    //The enemy spawn_timer
    float spawn_timer = ENEMY_SPAWN_TIMER;

    double accumulated_time = 0;

    double last_time = GetTime();

    Font font = LoadFont("Fonts/NotoSans-VariableFont.ttf");

    Initialize_Agents();

    coordinate_prob = calloc(NUM_OF_AGENTS, sizeof(Coordinate_Probability));

    for(int i = 0; i < NUM_OF_AGENTS; i++){
        Init_Coord_Prob(&coordinate_prob[i]);
        Read_Data(&coordinate_prob[i], i);
    }

    for(int i = 0; i < NUM_OF_AGENTS; i++){

        ship_ctrl_prob[i] = calloc(1, sizeof(Ship_Ctrl_Probability));

        Init_Ship_Ctrl_Prob(ship_ctrl_prob[i]);
    }

    best_target_probability = calloc(NUM_OF_AGENTS, sizeof *best_target_probability);

    while (!WindowShouldClose()) {

        //closest_enemy = NULL;
        for(int i = 0; i < NUM_OF_AGENTS; i++){
            best_target_probability[i] = NULL;
        }

        double this_time = GetTime();

        double delta_time = this_time - last_time;

        current_time += delta_time;

        spawn_timer -= delta_time;

        int* keys_down = calloc(NUM_OF_AGENTS, sizeof(int));


        last_time = this_time;

        //The enemy spawn spawn_timer
        if(spawn_timer <= 0.0f){

            int choose = GetRandomValue(0,2);

            Enemy *enemy = enemy_variants[choose]();

            Add_Enemy(enemy);

            spawn_timer = ENEMY_SPAWN_TIMER;
        }


        for(int i = 0; i < NUM_OF_AGENTS; i++){

            if(spaceships[i]->dead){
                continue;
            }

            for_spaceship = i;
            closest_enemy = NULL;
            best_target_probability[i] = NULL;

            //Iterate_Enemies(Gather_State, delta_time, enemy_collections);
            Iterate_Enemies(Find_Best_Probability_Target, delta_time, enemy_collections);
            Iterate_Enemies(Detect_Spaceship_Collision, delta_time, enemy_collections);

            //spaceships[i]->locked_target = best_target_probability[i];

            if(best_target_probability[i] == NULL){

                Help_Ally(i);

            } 

            spaceships[i]->locked_target = best_target_probability[i];

            Iterate_Enemies(Detect_Surrounding_Enemies, delta_time, enemy_collections);
            
            //Find the closest surrounding enemy if any
            Iterate_Enemies(Find_Closest_Enemy, delta_time, spaceships[i]->surrounding_enemies);

            Vector2 displacement = {0, 0};
            int distance = 0;

            
            if (spaceships[i]->locked_target != NULL) {

                displacement = Calc_XY_Coord_Difference(
                    spaceships[i]->position,
                    spaceships[i]->locked_target->position
                );

                Vector2 spaceship_center = Find_Center(spaceships[i]->position, spaceships[i]->size_r);
                Vector2 target_center = Find_Center(spaceships[i]->locked_target->position, spaceships[i]->locked_target->size_r);

                distance = (int)calc_mag(spaceship_center, target_center);

                distance /= MAP_FACTOR;

                distance = distance >= distance_size ? distance_size - 1 : distance;

            }
                

            //spaceships[i]->learning_cooldown -= delta_time;
            //spaceships[i]->avoid_enemy_timer -= delta_time;

            
            if(spaceships[i]->locked_target != NULL){

                //bool approach = Compute_Approach(&coordinate_prob[i].approach_prob[distance]);

                Learn_To_Approach(spaceships[i]->locked_target, spaceships[i], closest_enemy == NULL);
                //spaceships[i]->learning_cooldown = LEARNING_CD;

            }
            

            Vector2 coord = Normalize(displacement);


            int x = (int)coord.x;
            int y = (int)coord.y;

            spaceships[i]->scp->horizontal = coordinate_prob[i].move_prob_h[x][y];
            spaceships[i]->scp->vertical = coordinate_prob[i].move_prob_v[x][y];

            spaceships[i]->scp->shoot = coordinate_prob[i].shoot_prob[x][y];

            keys_down[i] = Compute_Action(spaceships[i]->scp);
            
        }

        if(frame_count % SNAPSHOT_INTERVAL_FRAMECOUNT == 0){

            for(int i = 0; i < NUM_OF_AGENTS; i++){

                Create_Snapshot_Of_Enemies_For_Spaceship(spaceships[i], &keys_down[i], delta_time);

            }

        }

        for(int i = 0; i < NUM_OF_AGENTS; i++){

            if(spaceships[i]->dead){
                continue;
            }

            Control_Spaceship(i, keys_down[i], delta_time);

        }

        Data* data = Create_Snapshot(spaceships, frame_count, score, delta_time, keys_down);

        current_game_snapshot = data;


        BeginDrawing();

            ClearBackground(BLACK);

            for(int i = 0; i < NUM_OF_AGENTS; i++){

                Spaceship *spaceship = spaceships[i];

                DrawTexture(spaceship->model, spaceship->position.x, spaceship->position.y, WHITE);

                char index[16];

                snprintf(index, sizeof(index), "%i", i);
                DrawTextEx(font, index, (Vector2){spaceship->position.x + spaceship->size_r/2 + 10, spaceship->position.y + spaceship->size_r + 15}, 24, 1, WHITE);
            }

            Iterate_Projectiles(Detect_Projectile_Collisions, delta_time);


            Iterate_Enemies(Move_Enemy, delta_time, enemy_collections);
            Iterate_Enemies(Update_Enemy_Attack_Cooldown, delta_time, enemy_collections);
            Iterate_Projectiles(Move_Projectile, delta_time);


            char score_text[100];
            char health_text[25];
            char time_elapsed[25];
            char hit_rate[25];

            for(int i = 0; i < NUM_OF_AGENTS; i++){
                snprintf(score_text, 100, "Ship %i Total Score: %d", i, score[i]);

                DrawTextEx(font, score_text, (Vector2){50, 50 + 50*i}, 24, 1, WHITE);

                snprintf(health_text, 25, "%i Health: %i", i, (int)spaceships[i]->health);

                DrawTextEx(font, health_text, (Vector2){WINDOW_WIDTH - 300, 50 + 50*i}, 24, 1, WHITE);
            }

            

            snprintf(time_elapsed, 25, "Time: %d", ((int)accumulated_time));
            snprintf(hit_rate, 25, "Hit Rate: %i%%", (int)(100 * ((float)enemies_hit/(float)enemies_spawned)));

            DrawTextEx(font, time_elapsed, (Vector2){WINDOW_WIDTH/2, 50}, 24, 1, WHITE);
            DrawTextEx(font, hit_rate, (Vector2){WINDOW_WIDTH/2, 100}, 24, 1, WHITE);

        EndDrawing();

        bool keep_going = false;

        for(int i = 0; i < NUM_OF_AGENTS; i++){

            spaceships[i]->surrounding_enemies->head = NULL;
            spaceships[i]->surrounding_enemies->tail = NULL;
            spaceships[i]->surrounding_enemies->count = 0;

            if(spaceships[i]->dead == false){
                keep_going = true;
            }

        }

        if(!keep_going){
            break;
        }


        frame_count++;

        accumulated_time += delta_time * 1000;

    }

    for(int i = 0; i < NUM_OF_AGENTS; i++){
        Save(&coordinate_prob[i], i);
    }

    UnloadImage(enemy_img_1);
    UnloadImage(enemy_img_2);
    UnloadImage(enemy_img_3);
    UnloadImage(ammo_image);
    UnloadImage(explosion_img);

    UnloadTexture(enemy_model_1);
    UnloadTexture(enemy_model_2);
    UnloadTexture(enemy_model_3);
    UnloadTexture(ammo_model);
    UnloadTexture(explosion_model);

    for(int i =0; i < NUM_OF_AGENTS; i++){
        Spaceship *spaceship = spaceships[i];

        UnloadTexture(spaceship->model);
    }

    UnloadFont(font);

    CloseWindow();

    return 0;
}
