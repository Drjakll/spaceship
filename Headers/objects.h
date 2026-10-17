struct Ammo {
    Vector2 position;
    float damage;
    Texture model;
    void (*Trajectory)(Vector2*, float, Vector2*, float);
    float dmg_area_radius;
    Vector2 *velocity;
    float acceleration;
    float size_r;
    int belongs_to;
    long id;
    double time_spawned;
    Vector2 initial_spaceship_pos;
    Data *end_snapshot;
    Data *start_snapshot;
    long locked_target_id;
};

struct Weapon {
    float cooldown; //time delay before next shot is allowed
    float cd_remain;
    Ammo* (*Shoot)(float, Vector2, int);

};

struct Enemy {
    char *name;
    Vector2 position;
    int points_worth;
    Vector2 *velocity;
    float acceleration;
    float health;
    float damage;
    Texture model;
    void (*FlightPath)(Vector2*, float, Vector2*, float, Enemy*);
    float size_r;
    bool dead;
    float attack_cooldown;
    float attack_cd_at;
    double time_spawned;
    float* time_velocity_shifted;
    int time_shifted_count;
    long id;
    float avoidence_cooldown;
};

struct Spaceship_State {
    int enemies_dist_left;
    int enemies_dist_right;
    int enemies_dist_top;
    int enemies_dist_bottom;

};

struct Spaceship {
    Vector2 position;
    Weapon* weapon;
    float health;
    float speed;
    Texture model;
    float size_r;
    Ship_Ctrl_Probability *scp;
    Spaceship_State state;
    Enemy *locked_target;
    float avoid_enemy_timer;
    float learning_cooldown;
    bool dead;
    Enemy_Collections* surrounding_enemies; 
};

float* Increase_Array_size(float *arr, float new_val, int old_size){

    float *new_arr = calloc(old_size+1, sizeof(float));

    for(int i = 0; i < old_size; i++){
        new_arr[i] = arr[i];
    }

    new_arr[old_size] = new_val;

    return new_arr;
}

void AccelerateStraightVPath(Vector2 *pos, float deltaTime, Vector2 *velocity, float acceleration){

    velocity->y += acceleration * deltaTime;

    pos->y -= velocity->y * deltaTime;

}

void AccelerateZigZagVPath(Vector2 *pos, float deltaTime, Vector2 *velocity, float acceleration, Enemy *e){

    velocity->y += acceleration * deltaTime;
    
    pos->y +=  velocity->y * deltaTime;

    //velocity->x += ((velocity->x / fabs(velocity->x)) * acceleration * deltaTime);

    int c = (int)(floor(pos->x)) % 350;

    if( c < 10 && c > 0 ){
        //e->time_velocity_shifted = Increase_Array_size(e->time_velocity_shifted, current_time, e->time_shifted_count);
        velocity->x *= -1;
    }

    pos->x += (velocity->x * deltaTime);

}

Enemy *Aliencraft_Type_1(){

    Enemy *enemy = calloc(1, sizeof(Enemy));

    Vector2 *init_vel = calloc(1, sizeof(Vector2));

    char *name = ENEMY_TYPE_1;

    init_vel->y = 10.0f;
    init_vel->x = ENEMY_AVOID_NORMAL_SPEED;

    enemy->position = (Vector2){GetRandomValue(0, 800), -ENEMY_RADIUS};
    enemy->points_worth =  2 ;
    enemy->velocity = init_vel;
    enemy->health = ENEMY_HEALTH * 0.5f;
    enemy->damage = 5;
    enemy->size_r = ENEMY_RADIUS;
    enemy->acceleration = ENEMY_VERTICAL_ACCELERATION;
    enemy->FlightPath = AccelerateZigZagVPath;
    enemy->name = name;
    enemy->dead = false;
    enemy->attack_cooldown = ENEMY_TYPE_1_ATTACK_CD;
    enemy->attack_cd_at = 0.0f;
    enemy->time_spawned = current_time;
    enemy->time_velocity_shifted = calloc(1, sizeof(float));
    enemy->time_shifted_count = 0;
    enemy->avoidence_cooldown = ENEMY_AVOIDENCE_CD;


    enemy->model = enemy_model_1;

    return enemy;
}

Enemy *Aliencraft_Type_2(){

    Enemy *enemy = calloc(1, sizeof(Enemy));

    Vector2 *init_vel = calloc(1, sizeof(Vector2));

    char *name = ENEMY_TYPE_2;

    init_vel->y = 10.0f;
    init_vel->x = ENEMY_AVOID_NORMAL_SPEED;

    enemy->position = (Vector2){GetRandomValue(0, 800), -ENEMY_RADIUS};
    enemy->points_worth = 4 ;
    enemy->velocity = init_vel;
    enemy->health = ENEMY_HEALTH * 0.75f;
    enemy->damage = 7;
    enemy->size_r = ENEMY_RADIUS;
    enemy->acceleration = ENEMY_VERTICAL_ACCELERATION * 0.8;
    enemy->FlightPath = AccelerateZigZagVPath;
    enemy->name = name;
    enemy->dead = false;
    enemy->attack_cooldown = ENEMY_TYPE_2_ATTACK_CD;
    enemy->attack_cd_at = 0.0f;
    enemy->time_spawned = current_time;
    enemy->time_velocity_shifted = calloc(1, sizeof(float));
    enemy->time_shifted_count = 0;
    enemy->avoidence_cooldown = ENEMY_AVOIDENCE_CD;

    enemy->model = enemy_model_2;

    return enemy;
}

Enemy *Aliencraft_Type_3(){

    Enemy *enemy = calloc(1, sizeof(Enemy));

    Vector2 *init_vel = calloc(1, sizeof(Vector2));

    char *name = ENEMY_TYPE_3;

    init_vel->y = 10.0f;
    init_vel->x = ENEMY_AVOID_NORMAL_SPEED;

    enemy->position = (Vector2){GetRandomValue(0, 800), -ENEMY_RADIUS};
    enemy->points_worth = 8 ;
    enemy->velocity = init_vel;
    enemy->health = ENEMY_HEALTH;
    enemy->damage = 10;
    enemy->size_r = ENEMY_RADIUS;
    enemy->acceleration = ENEMY_VERTICAL_ACCELERATION * 0.6;
    enemy->FlightPath = AccelerateZigZagVPath;
    enemy->name = name;
    enemy->dead = false;
    enemy->attack_cooldown = ENEMY_TYPE_3_ATTACK_CD;
    enemy->attack_cd_at = 0.0f;
    enemy->time_spawned = current_time;
    enemy->time_velocity_shifted = calloc(1, sizeof(float));
    enemy->time_shifted_count = 0;
    enemy->avoidence_cooldown = ENEMY_AVOIDENCE_CD;

    enemy->model = enemy_model_3;

    return enemy;
}



Ammo* ShootMissile(float cooldown, Vector2 init_pos, int belongs_to){

    if(cooldown > 0){
        return NULL;
    }
    
    Ammo* ammo = calloc(1, sizeof(Ammo));

    Vector2 *init_vel = calloc(1, sizeof(Vector2));

    init_vel->y = 30.0f;
    init_vel->x = 0.0f;
    
    ammo->position = init_pos; 
    ammo->damage = 10.0f;
    ammo->model = ammo_model;
    ammo->dmg_area_radius = EXPLOSION_RADIUS;
    ammo->Trajectory = AccelerateStraightVPath;
    ammo->size_r = MISSILE_RADIUS;
    ammo->velocity = init_vel;
    ammo->acceleration = 300.0f;
    ammo->belongs_to = belongs_to;
    ammo->time_spawned = current_time;
    ammo->start_snapshot = current_game_snapshot;
    ammo->initial_spaceship_pos = spaceships[belongs_to]->position;
    Enemy *target = spaceships[belongs_to]->locked_target;
    ammo->locked_target_id = target ? target->id : 0;

    return ammo;
}

Spaceship *Create_Spaceship() {

    Spaceship *spaceship = calloc(1, sizeof(Spaceship));

    Weapon* defaultWeapon = calloc(1, sizeof(Weapon));

    defaultWeapon->cooldown = 0.25f;
    defaultWeapon->cd_remain = 0.0f;
    defaultWeapon->Shoot = ShootMissile;

    spaceship->health = SPACESHIP_HEALTH;
    spaceship->speed = SPACESHIP_SPEED;
    spaceship->weapon = defaultWeapon;
    spaceship->size_r = SPACESHIP_RADIUS;
    spaceship->state = (Spaceship_State){
                        .enemies_dist_left = 0, 
                        .enemies_dist_right = 0,
                        .enemies_dist_top = 0,
                        .enemies_dist_bottom = 0};
    spaceship->avoid_enemy_timer = 0.0f;
    spaceship->learning_cooldown = 0.0f;
    spaceship->locked_target = NULL;
    spaceship->scp = calloc(1, sizeof(Ship_Ctrl_Probability));
    spaceship->dead = false;
    spaceship->surrounding_enemies = calloc(1, sizeof(Enemy_Collections));
    Init_Ship_Ctrl_Prob(spaceship->scp);

    Image img = LoadImage("Pictures/spaceship.png");

    ImageResize(&img, SPACESHIP_RADIUS * 2, SPACESHIP_RADIUS * 2);

    spaceship->model = LoadTextureFromImage(img);
    UnloadImage(img);

    return spaceship;
}