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
} ;

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
    void (*FlightPath)(Vector2*, float, Vector2*, float);
    float size_r;
    bool dead;
    float attack_cooldown;
    float attack_cd_at;
};

struct Spaceship {
    Vector2 position;
    Weapon* weapon;
    float health;
    float speed;
    Texture model;
    float size_r;
};



void AccelerateStraightVPath(Vector2 *pos, float deltaTime, Vector2 *velocity, float acceleration){

    velocity->y += acceleration * deltaTime;

    pos->y -=  velocity->y * deltaTime;

}

void AccelerateZigZagVPath(Vector2 *pos, float deltaTime, Vector2 *velocity, float acceleration){

    velocity->y += acceleration * deltaTime;
    
    pos->y +=  velocity->y * deltaTime;

    //velocity->x += ((velocity->x / fabs(velocity->x)) * acceleration * deltaTime);

    int c = (int)(floor(pos->x)) % 350;

    if( c < 10 && c > 0 ){
        velocity->x *= -1;
    }

    pos->x += (velocity->x * deltaTime);

}

Enemy *Aliencraft_Type_1(){

    Enemy *enemy = calloc(1, sizeof(Enemy));

    Vector2 *init_vel = calloc(1, sizeof(Vector2));

    char *name = ENEMY_TYPE_1;

    init_vel->y = 10.0f;
    init_vel->x = 100.0f;

    enemy->position = (Vector2){GetRandomValue(0, 800), -ENEMY_RADIUS};
    enemy->points_worth =  2 ;
    enemy->velocity = init_vel;
    enemy->health = 10.0f;
    enemy->damage = 5;
    enemy->size_r = ENEMY_RADIUS;
    enemy->acceleration = 75.0f;
    enemy->FlightPath = AccelerateZigZagVPath;
    enemy->name = name;
    enemy->dead = false;
    enemy->attack_cooldown = ENEMY_TYPE_1_ATTACK_CD;
    enemy->attack_cd_at = 0.0f;


    enemy->model = enemy_model_1;

    return enemy;
}

Enemy *Aliencraft_Type_2(){

    Enemy *enemy = calloc(1, sizeof(Enemy));

    Vector2 *init_vel = calloc(1, sizeof(Vector2));

    char *name = ENEMY_TYPE_2;

    init_vel->y = 10.0f;
    init_vel->x = 150.0f;

    enemy->position = (Vector2){GetRandomValue(0, 800), -ENEMY_RADIUS};
    enemy->points_worth = 4 ;
    enemy->velocity = init_vel;
    enemy->health = 15.0f;
    enemy->damage = 7;
    enemy->size_r = ENEMY_RADIUS;
    enemy->acceleration = 50.0f;
    enemy->FlightPath = AccelerateZigZagVPath;
    enemy->name = name;
    enemy->dead = false;
    enemy->attack_cooldown = ENEMY_TYPE_2_ATTACK_CD;
    enemy->attack_cd_at = 0.0f;

    enemy->model = enemy_model_2;

    return enemy;
}

Enemy *Aliencraft_Type_3(){

    Enemy *enemy = calloc(1, sizeof(Enemy));

    Vector2 *init_vel = calloc(1, sizeof(Vector2));

    char *name = ENEMY_TYPE_3;

    init_vel->y = 10.0f;
    init_vel->x = 100.0f;

    enemy->position = (Vector2){GetRandomValue(0, 800), -ENEMY_RADIUS};
    enemy->points_worth = 8 ;
    enemy->velocity = init_vel;
    enemy->health = 25.0f;
    enemy->damage = 10;
    enemy->size_r = ENEMY_RADIUS;
    enemy->acceleration = 20.0f;
    enemy->FlightPath = AccelerateZigZagVPath;
    enemy->name = name;
    enemy->dead = false;
    enemy->attack_cooldown = ENEMY_TYPE_3_ATTACK_CD;
    enemy->attack_cd_at = 0.0f;

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

    Image img = LoadImage("Pictures/spaceship.png");

    ImageResize(&img, SPACESHIP_RADIUS * 2, SPACESHIP_RADIUS * 2);

    spaceship->model = LoadTextureFromImage(img);
    UnloadImage(img);

    return spaceship;
}