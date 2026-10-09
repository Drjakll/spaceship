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

    Insert_Enemy(enemy);
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

//Detecting each enemy information and calculate certain data

bool Detect_Enemy_Locations(Enemy *enemy, double deltaTime){

    for(int i = 0; i < NUM_OF_AGENTS; i++){

        

    }

    return false;
}

bool Detect_Enemy_Collisions(Enemy *enemy, double deltaTime){

    float dist = calc_mag(current_ammo->position, enemy->position);

    float collision_min = current_ammo->size_r + enemy->size_r;

    if(dist < collision_min && !enemy->dead){
        
        enemy->health -= current_ammo->damage;

        if(enemy->health <= 0.0f){
            enemy->model = explosion_model;
            enemy->velocity->y = 50.0f;
            enemy->velocity->x = 1.0f;
            enemy->acceleration = 0.0f;
            enemy->size_r = EXPLOSION_RADIUS;
            enemy->damage = 1.0f;
            enemy->attack_cooldown = EXPLOSION_DMG_COOLDOWN;
            enemy->dead = true;

            score[current_ammo->belongs_to] += enemy->points_worth;
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

    bool collision_detected = Iterate_Enemies(Detect_Enemy_Collisions, deltaTime);

    current_ammo = NULL;

    return collision_detected;
}

bool Move_Enemy(Enemy *enemy, double deltaTime){

    enemy->FlightPath(&enemy->position, deltaTime, enemy->velocity, enemy->acceleration);

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

bool Detect_Spaceship_Collision(Enemy *enemy, double deltaTime, Spaceship *spaceship){

    float dist = calc_mag(spaceship->position, enemy->position);

    float collision_min = spaceship->size_r + enemy->size_r;

    if(dist < collision_min && enemy->attack_cd_at <= 0.0f){
        spaceship->health -= enemy->damage;
        enemy->attack_cd_at = enemy->attack_cooldown;
    }

    if(spaceship->health <= 0.0f){
        return true;
    }

    return false;
}