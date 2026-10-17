long Generate_Projectile_Index() {

    return ++ammo_index;
}

//Add a projectile to the linked list
void Add_Projectile(Ammo *ammo){

    if(!ammo){
        return;
    }

    ammo->id = Generate_Projectile_Index();

    //This insert it to the linked list, this is from object_collection.h
    Insert_Projectile(ammo);
}

void Add_Enemy(Enemy *enemy){

    if(!enemy){
        return;
    }

    enemies_spawned++;

    Insert_Enemy(enemy, enemy_collections);
}

float calc_mag(Vector2 v1, Vector2 v2){

    Vector2 diff = {v2.x - v1.x, v2.y - v1.y};

    return sqrt(diff.x * diff.x + diff.y * diff.y);

}

float* Distance_Hori_Vert(Enemy enemy, Spaceship spaceship) {

    Vector2 e_loc = enemy.position;
    Vector2 s_loc = spaceship.position;

    float* differences = malloc(sizeof(float)*2);

    differences[0] = s_loc.x - e_loc.x;
    differences[1] = s_loc.y - e_loc.y;

    return differences;
}

Vector2 Normalize(Vector2 disp){

    int limit_x = 2 * WINDOW_WIDTH/MAP_FACTOR;
    int limit_y = 2 * WINDOW_HEIGHT/MAP_FACTOR;

    disp.x /= MAP_FACTOR;
    disp.y /= MAP_FACTOR;

    disp.x += (limit_x/2 - 1);
    disp.y += (limit_y/2 - 1);

    disp.x = disp.x < 0 ? 0 : disp.x;
    disp.y = disp.y < 0 ? 0 : disp.y;

    disp.x = disp.x >= limit_x ? limit_x - 1 : disp.x;
    disp.y = disp.y >= limit_y ? limit_y - 1 : disp.y;

    return disp;
}

void Surface_Area_Shoot(int radius, Ship_Ctrl_Shoot_Prob scsp, Vector2 center){


    for(int i = center.x - radius; i < center.x + radius; i++){

        if(i < 0 || i >= WINDOW_WIDTH/MAP_FACTOR){
            continue;
        }

        for(int j = center.y - radius; j < center.y + radius; j++){

            if(j < 0 || j >= WINDOW_HEIGHT/MAP_FACTOR){
                continue;
            }

            coordinate_prob->shoot_prob[i][j] = scsp;

        }
    }
}

void Surface_Area_Move(int radius, Ship_Ctrl_Move_Prob scmp_, Vector2 center, Ship_Ctrl_Move_Prob (*scmp)[MAX_HORIZONTAL * WINDOW_WIDTH/MAP_FACTOR]){

    for(int i = center.x - radius; i < center.x + radius; i++){

        if(i < 0 || i >= MAX_HORIZONTAL * WINDOW_WIDTH/MAP_FACTOR){
            continue;
        }

        for(int j = center.y - radius; j < center.y + radius; j++){

            if(j < 0 || j >= MAX_VERTICAL * WINDOW_HEIGHT/MAP_FACTOR){
                continue;
            }

            scmp[i][j] = scmp_;
        }
    }
}

void Learn(Enemy_Data *enemy_data, Spaceship_Data* spaceship_data, Ammo *ammo, float shoot_answer[], float h_answer[], float v_answer[]){

    if(ammo)
        ammo->end_snapshot = current_game_snapshot;

    Vector2 displacement = {.x = 0, .y = 0};

    if(!enemy_data){
        return;
    }

    if(ammo)
        displacement = Calc_XY_Coord_Difference(ammo->initial_spaceship_pos, enemy_data->position);

    if(spaceship_data)
        displacement = Calc_XY_Coord_Difference(spaceship_data->position, enemy_data->position);

    int limit_x =  2 * WINDOW_WIDTH/(MAP_FACTOR);
    int limit_y =  2 * WINDOW_HEIGHT/(MAP_FACTOR); 

    displacement.x /= (int)(MAP_FACTOR);
    displacement.y /= (int)(MAP_FACTOR);

    displacement.x = (int)(displacement.x + limit_x/2) - 1;
    displacement.y = (int)(displacement.y + limit_y/2) - 1;
    
    int x = (int)displacement.x;
    int y = (int)displacement.y;

    x = x < 0 ? 0 : x;
    y = y < 0 ? 0 : y;

    x = x >= limit_x ? limit_x - 1 : x;
    y = y >= limit_y ? limit_y - 1 : y;

    int coord_index = ammo ? ammo->belongs_to : for_spaceship;


    Ship_Ctrl_Shoot_Prob* shoot_prob = &coordinate_prob[coord_index].shoot_prob[x][y];

    float input[] = {(2.0f)* (x / (float)(limit_x)), (2.0f)*(y / (float)(limit_y))};

    Multi_Logistic_Regression(shoot_prob->shoot, learning_rate, shoot_answer, input, 2, 2);



    Ship_Ctrl_Move_Prob* scmp_h = &coordinate_prob[coord_index].move_prob_h[x][y];
    Ship_Ctrl_Move_Prob* scmp_v = &coordinate_prob[coord_index].move_prob_v[x][y];


    Multi_Logistic_Regression(scmp_h->move, learning_rate, h_answer, input, 3, 2);
    Multi_Logistic_Regression(scmp_v->move, learning_rate, v_answer, input, 3, 2);

    int s = shoot_prob->shoot[0].probability * 100;
    int ds = shoot_prob->shoot[1].probability * 100;

    int l = scmp_h->move[0].probability * 100;
    int sh = scmp_h->move[1].probability * 100;
    int r = scmp_h->move[2].probability * 100;

    int u = scmp_v->move[0].probability * 100;
    int sv = scmp_v->move[1].probability * 100;
    int d = scmp_v->move[2].probability * 100;

    if(ammo){
        printf("Enemy Coord- x: %i, y: %i\nSpaceship Coord- x: %i, y: %i\n\n", 
                                                (int)enemy_data->position.x/MAP_FACTOR,
                                                (int)enemy_data->position.y/MAP_FACTOR,
                                                (int)ammo->initial_spaceship_pos.x/MAP_FACTOR,
                                                (int)ammo->initial_spaceship_pos.y/MAP_FACTOR 
                                            );
        printf("Disp Coord - x: %i, y: %i\nshoot: %i%%, no shoot: %i%%\nleft: %i%%, stay: %i%%, right: %i%%\nup: %i%%, stay: %i%%, down: %i%%\n\n", x, y, s, ds, l, sh, r, u, sv, d);
    }

    if(spaceship_data){
        printf("Enemy Coord- x: %i, y: %i\nSpaceship Coord- x: %i, y: %i\n\n", 
                                                (int)enemy_data->position.x/MAP_FACTOR,
                                                (int)enemy_data->position.y/MAP_FACTOR,
                                                (int)spaceship_data->position.x/MAP_FACTOR,
                                                (int)spaceship_data->position.y/MAP_FACTOR 
                                            );
        printf("Disp Coord - x: %i, y: %i\nshoot: %i%%, no shoot: %i%%\nleft: %i%%, stay: %i%%, right: %i%%\nup: %i%%, stay: %i%%, down: %i%%\n\n", x, y, s, ds, l, sh, r, u, sv, d);
    }
}

void Learn_To_Shoot(long enemy_id, Ammo *ammo, bool to_shoot){

    if(!enemy_id || !ammo->start_snapshot){
        return;
    }

    Enemy_Data* enemy_data = Find_Target_Enemy_Data(ammo->start_snapshot->enemy_data_list, enemy_id);

    if(!enemy_data){
        return;
    }

    int belongs_to = ammo->belongs_to;

    int keydown = ammo->start_snapshot->keys_down[belongs_to];

    if(to_shoot){
        printf("shoot!\n");
    } else {
        printf("don't shoot!\n");
    }

    float shoot_answer[] = {to_shoot ? 1 : 0, to_shoot ? 0 : 1};
    float h_answer[] = {0, 0, 0};
    float v_answer[] = {0, 0, 0};

    if(keydown & LEFT){
        h_answer[to_shoot ? 0 : 2] = 1;
    } else if(keydown & RIGHT){
        h_answer[to_shoot ? 2 : 0] = 1;
    } else {
        h_answer[1] = 1;
    }

    if(keydown & UP){
        v_answer[to_shoot ? 0 : 2] = 1;
    } else if(keydown & DOWN){
        v_answer[to_shoot ? 2 : 0] = 1;
    } else {
        v_answer[1] = 1;
    }

    Learn(enemy_data, NULL, ammo, shoot_answer, h_answer, v_answer);

}


void Learn_To_Approach(Enemy *enemy, Spaceship *spaceship, bool approach){

    bool avoid = !approach;

    if(!enemy){
        return;
    }

    //printf("approaching %li\n", enemy->id);

    if(avoid){
        //spaceship->avoid_enemy_timer = 3.0f;
    }

    Vector2 disp = Calc_XY_Coord_Difference(spaceship->position, enemy->position);

    Vector2 coord = Normalize(disp);

    float limit_x = 2 * WINDOW_WIDTH/MAP_FACTOR;
    float limit_y = 2 * WINDOW_HEIGHT/MAP_FACTOR;


    int x = (int)(coord.x);
    int y = (int)(coord.y);

    int distance = (int)calc_mag(spaceship->position, enemy->position);
    distance /= MAP_FACTOR;

    distance = distance >= distance_size ? distance_size - 1 : distance;

    float input[] = { 2.0f * (x) / (limit_x), 2.0f* (y) / (limit_y)};

    Ship_Ctrl_Move_Prob* scmp_h = &coordinate_prob[for_spaceship].move_prob_h[x][y];
    Ship_Ctrl_Move_Prob* scmp_v = &coordinate_prob[for_spaceship].move_prob_v[x][y];
    Ship_Ctrl_Approach_Prob* approach_prob = &coordinate_prob[for_spaceship].approach_prob[distance];
    
    float h_answer[] = {0,0,0};
    float v_answer[] = {0,0,0};

    if(!avoid){
        avoid = scmp_h->avoid_zone;
    }

    if(disp.x > 0){ //Enemy at the left side
        h_answer[avoid ? 2 : 0] = 1;
        x = avoid ? x + 1 : x;
    } else if(disp.x < 0){ //Enemy at the right side
        h_answer[avoid ? 0 : 2] = 1;
        x = avoid ? x - 1 : x;
    } else {
        h_answer[avoid ? 2 : 1] = 1;
        x = avoid ? x + 1 : x;
    }

    if(disp.y > 0){ //Enemy at the front
        v_answer[avoid ? 2 : 0] = 1;
        y = avoid ? y + 1 : y;
    } else if(disp.y < 0){ //Enemy at the back
        v_answer[avoid ? 0 : 2] = 1;
        y = avoid ? y - 1 : y;
    } else {
        v_answer[avoid ? 2 : 1] = 1;
        y = avoid ? y + 1 : y;
    }


    float awareness = (float)distance/(ENEMY_AWARENESS_RADIUS/MAP_FACTOR);

    float avoidence = fmaxf(0.0f, 1.0f - awareness);    

    float approach_input[] = {(float)distance/distance_size};
    float approach_answer[] = {1.0f - avoidence,  avoidence};
    

    
    Multi_Logistic_Regression(scmp_h->move, learning_rate, h_answer, input, 3, 2);
    Multi_Logistic_Regression(scmp_v->move, learning_rate, v_answer, input, 3, 2);
    Multi_Logistic_Regression(approach_prob->approach, learning_rate, approach_answer, approach_input, 2, 1);


    //Being more cautious about getting close to the enemy target if avoid tag is triggered
    /*
    if(avoid){
        scmp_h->avoid_zone = true;
        scmp_v->avoid_zone = true;

        x = x < 0 ? 0 : x;
        x = x >= limit_x ? limit_x - 1 : x;

        y = y < 0 ? 0 : y;
        y = y >= limit_y ? limit_y - 1 : y;

        scmp_h = &coordinate_prob[for_spaceship].move_prob_h[x][y];
        scmp_v = &coordinate_prob[for_spaceship].move_prob_v[x][y];

        input[0] = 1.0f - ((float)x/limit_x);
        input[1] = 1.0f - ((float)y/limit_y);

        Multi_Logistic_Regression(scmp_h->move, lr, h_answer, input, 3, 2);
        Multi_Logistic_Regression(scmp_v->move, lr, v_answer, input, 3, 2);

        scmp_h->avoid_zone = true;
        scmp_v->avoid_zone = true;
    }*/

}

bool Enemy_Learn_To_Avoid(Enemy *enemy, double deltaTime){

    if(enemy->dead){
        return false;
    }

    if(enemy->avoidence_cooldown > 0){
        enemy->avoidence_cooldown -= deltaTime;
        return false;
    }

    float sum_prob = 0.0f;

    for(int i = 0; i < NUM_OF_AGENTS; i++){

        Spaceship *current_ship = spaceships[i];

        Vector2 disp = Calc_XY_Coord_Difference(current_ship->position, enemy->position);

        disp = Normalize(disp);

        Ship_Ctrl_Shoot_Prob *shoot_prob = &coordinate_prob[i].shoot_prob[(int)disp.x][(int)disp.y];

        float hit_probability = shoot_prob->shoot[0].probability;

        sum_prob += hit_probability;

    }

    if(sum_prob/NUM_OF_AGENTS > 0.05f){

        enemy->avoidence_cooldown = ENEMY_AVOIDENCE_CD;
        
        if(enemy->velocity->x > 0){

            enemy->velocity->x += ENEMY_AVOID_SPEED_INCREMENT;

            enemy->velocity->x = enemy->velocity->x > 800 ? 800 : enemy->velocity->x;

        } else {

            enemy->velocity->x -= ENEMY_AVOID_SPEED_INCREMENT;

            enemy->velocity->x = enemy->velocity->x < -800 ? -800 : enemy->velocity->x;
        }

        enemy->velocity->x *= -1;
    } else {
        enemy->velocity->x = 0.0f;
    }

    return false;

}

bool Detect_Enemy_Collisions(Enemy *enemy, double deltaTime){

    float dist = calc_mag(current_ammo->position, enemy->position);

    float collision_min = current_ammo->size_r + enemy->size_r;

    if(dist < collision_min && !enemy->dead){
        enemy->health -= current_ammo->damage;

        
        Learn_To_Shoot(enemy->id, current_ammo, true);

        if(enemy->health <= 0.0f){
            enemy->model = explosion_model;
            enemy->velocity->y = 100.0f;
            enemy->velocity->x = 1.0f;
            enemy->acceleration = -1.0f;
            enemy->size_r = EXPLOSION_RADIUS;
            enemy->damage = 1.0f;
            enemy->attack_cooldown = EXPLOSION_DMG_COOLDOWN;
            enemy->dead = true;
            
            score[current_ammo->belongs_to] += enemy->points_worth;
            enemies_hit++;
        }

        return true;
        
    }

    return false;

}

bool Detect_Projectile_Collisions(Ammo *ammo, double deltaTime){

    if(ammo == NULL){
        return false;
    }

    current_ammo = ammo;

    bool collision_detected = Iterate_Enemies(Detect_Enemy_Collisions, deltaTime, enemy_collections);

    current_ammo = NULL;

    return collision_detected;
}

bool Move_Enemy(Enemy *enemy, double deltaTime){

    enemy->FlightPath(&enemy->position, deltaTime, enemy->velocity, enemy->acceleration, enemy);

    enemy->position.x = enemy->position.x < 0 ? 0 : enemy->position.x;
    enemy->position.x = enemy->position.x > WINDOW_WIDTH - enemy->size_r*2 ? WINDOW_WIDTH - enemy->size_r*2 : enemy->position.x;

    DrawTexture(enemy->model, enemy->position.x, enemy->position.y, WHITE);

    return false;
}

bool Move_Projectile(Ammo *ammo, double deltaTime){

    ammo->Trajectory(&ammo->position, deltaTime, ammo->velocity, ammo->acceleration);

    DrawTexture(ammo->model, ammo->position.x, ammo->position.y, WHITE);

    return false;
}

bool Update_Enemy_Attack_Cooldown(Enemy *enemy, double deltaTime){

    enemy->attack_cd_at -= deltaTime;
    
    return false;
}


void Learn_What_Not_To_Do(Data* snapshot, long enemy_id){

    Enemy_Data* enemy_data = Find_Target_Enemy_Data(snapshot->enemy_data_list, enemy_id);

    int keys_down = *snapshot->keys_down;

    float shoot[] = {1,1};
    float move_h[] = {0.5, 0.5, 0.5};
    float move_v[] = {0.5, 0.5, 0.5};

    if(keys_down & SHOOT){
        shoot[1] = 0; 
    } else {
        shoot[0] = 0;
    }

    if(keys_down & LEFT){
        move_h[0] = 0.0;
    } else if(keys_down & RIGHT){
        move_h[2] = 0.0;
    } else {
        move_h[1] = 0.0;
    }

    if(keys_down & UP){
        move_v[0] = 0.0;
    } else if(keys_down & DOWN){
        move_v[2] = 0.0;
    } else {
        move_v[1] = 0.0f;
    }

    Learn(enemy_data, snapshot->spaceship_data, NULL, shoot, move_h, move_v);
}


bool Detect_Spaceship_Collision(Enemy *enemy, double deltaTime){

    Spaceship *spaceship = spaceships[for_spaceship];

    float dist = calc_mag(spaceship->position, enemy->position);

    float collision_min = spaceship->size_r + enemy->size_r;

    if(dist < collision_min && enemy->attack_cd_at <= 0.0f){
        spaceship->health -= enemy->damage;
        enemy->attack_cd_at = enemy->attack_cooldown;

    }

    if(dist < collision_min){
        //Learn_To_Approach(enemy, spaceships[for_spaceship], false);

        if(spaceship->snapshots && spaceship->snapshots->head){
            
            Data* snapshot = &spaceship->snapshots->head->data;

            Learn_What_Not_To_Do(snapshot, enemy->id);
        }
    }

    if(spaceship->health <= 0.0f){
        spaceship->dead = true;
        spaceship->position.x = -100;
        spaceship->position.y = -100;
        return true;
    }

    return false;
}

bool Gather_State(Enemy *enemy, double deltaTime ){

    Spaceship *ship = spaceships[for_spaceship];

    Vector2 displacement = Calc_XY_Coord_Difference(ship->position, enemy->position);

    Spaceship_State* state = &ship->state;

    state->enemies_dist_left += (displacement.x < 0 ? 0 : abs((int)displacement.x));
    state->enemies_dist_right += (displacement.x < 0 ? abs((int)displacement.x) : 0);
    state->enemies_dist_top += (displacement.y > 0 ? abs((int)displacement.y) : 0);
    state->enemies_dist_bottom += (displacement.y > 0 ? 0 : abs((int)displacement.y));

    return false;
}

bool Find_Closest_Enemy(Enemy *enemy, double deltaTime){

    if(enemy->dead){
        //return false;
    }

    if(closest_enemy == NULL){
        closest_enemy = enemy;
        return false;
    }

    Spaceship *spaceship = spaceships[for_spaceship];

    //Vector2 last_disp = Calc_XY_Coord_Difference(spaceship->position, closest_enemy->position);
    //Vector2 this_disp = Calc_XY_Coord_Difference(spaceship->position, enemy->position);

    float last_dist = calc_mag(spaceship->position, closest_enemy->position);
    float this_dist = calc_mag(spaceship->position, enemy->position);

    /*if(fabs(last_disp.x) < fabs(this_disp.x) && fabs(last_disp.y) > fabs(last_disp.y)){
        closest_enemy = enemy;
    }*/

    if(last_dist > this_dist){
        spaceships[for_spaceship]->locked_target = enemy;
    }

    return false;
}

bool Target_Taken(Enemy *target1, int i){

    if(i == for_spaceship || i < 0 || i > for_spaceship){
        return false;
    }

    Enemy *target2 = best_target_probability[i]; 

    return target1 == target2;

}

bool Find_Best_Probability_Target(Enemy *enemy, double deltaTime){

    if(!enemy || enemy->dead){
        return false;
    }

    for(int i = 0; i < NUM_OF_AGENTS; i++){

        if(Target_Taken(enemy, i)){
            return false;
        }

    }

    if(!best_target_probability[for_spaceship]){
        best_target_probability[for_spaceship] = enemy;
        return false;
    }

    Spaceship *spaceship = spaceships[for_spaceship];

    Vector2 old_disp = Calc_XY_Coord_Difference(spaceship->position, best_target_probability[for_spaceship]->position);
    Vector2 new_disp = Calc_XY_Coord_Difference(spaceship->position, enemy->position);

    float old_dist = sqrt(old_disp.x*old_disp.x + old_disp.y*old_disp.y);
    float new_dist = sqrt(new_disp.x*new_disp.x + new_disp.y*new_disp.y);

    old_disp = Normalize(old_disp);
    new_disp = Normalize(new_disp);
    
    Ship_Ctrl_Shoot_Prob old_shoot_prob = coordinate_prob[for_spaceship].shoot_prob[(int)old_disp.x][(int)old_disp.y];
    Ship_Ctrl_Shoot_Prob new_shoot_prob = coordinate_prob[for_spaceship].shoot_prob[(int)new_disp.x][(int)new_disp.y];

    float new_prob = new_shoot_prob.shoot[0].probability;
    float old_prob = old_shoot_prob.shoot[0].probability;

    float new_pts_worth = enemy->points_worth;
    float old_pts_worth = best_target_probability[for_spaceship]->points_worth;

    if( new_prob * new_pts_worth/(new_dist) > 
        old_prob * old_pts_worth/(old_dist)){
        best_target_probability[for_spaceship] = enemy;
    }

    return false;

}

bool Detect_Surrounding_Enemies(Enemy *enemy, double deltaTime){

    Spaceship* spaceship = spaceships[for_spaceship];

    float dist = calc_mag(spaceship->position, enemy->position);

    if(dist <= (enemy->size_r + spaceship->size_r + ROOM_TO_ESCAPE)){

        Insert_Enemy(enemy, spaceship->surrounding_enemies);
        //Learn_To_Approach(enemy, spaceship, false);
        //spaceship->locked_target = enemy;
    }
    
    return false;
}