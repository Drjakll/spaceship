typedef struct Enemy Enemy;
typedef struct Spaceship Spaceship;
typedef struct Ammo Ammo;
typedef struct Weapon Weapon;
typedef struct Projectile_Collections Projectile_Collections;
typedef struct Enemy_Collections Enemy_Collections;
typedef struct Projectile_Node Projectile_Node;
typedef struct Enemy_Node Enemy_Node;
typedef struct Enemy_Data Enemy_Data;
typedef struct Enemy_Data_Node Enemy_Data_Node;
typedef struct Enemy_Data_List Enemy_Data_List;
typedef struct Projectile_Data Projectile_Data;
typedef struct Projectile_Data_Node Projectile_Data_Node;
typedef struct Projectile_Data_List Projectile_Data_List;
typedef struct Spaceship_Data Spaceship_Data;
typedef struct Data Data;
typedef struct Data_Node Data_Node;
typedef struct Data_List Data_List;
typedef struct Reg_Data Reg_Data;
typedef struct Ship_Ctrl_Probability Ship_Ctrl_Probability;
typedef struct Ship_Ctrl_Move_Prob Ship_Ctrl_Move_Prob;
typedef struct Ship_Ctrl_Shoot_Prob Ship_Ctrl_Shoot_Prob;
typedef struct Coordinate_Probability Coordinate_Probability;
typedef struct Spaceship_State Spaceship_State;
typedef struct Ship_Ctrl_Approach_Prob Ship_Ctrl_Approach_Prob;

void Learn_To_Shoot(long, Ammo*, bool);


int score[NUM_OF_AGENTS] = {0};
int frame_count = 0;
int for_spaceship = 0;

Enemy_Collections *enemy_collections = NULL;
Projectile_Collections *projectiles = NULL;
Enemy_Data_List *current_enemy_data_list = NULL;
Data *current_game_snapshot = NULL;

Enemy *closest_enemy = NULL;
Enemy **best_target_probability = NULL;

Image explosion_img;
Texture explosion_model;

Ammo *current_ammo = NULL;

Data_List *data_list = NULL;

Image enemy_img_1;
Image enemy_img_2;
Image enemy_img_3;

Image explosion_img;

Texture enemy_model_1;
Texture enemy_model_2;
Texture enemy_model_3;

Image ammo_image;

Texture ammo_model;

Spaceship *spaceships[NUM_OF_AGENTS];
Ship_Ctrl_Probability *ship_ctrl_prob[NUM_OF_AGENTS];

Projectile_Data_List *current_projectile_data_list = NULL;

Coordinate_Probability* coordinate_prob = NULL;

int distance_size = 0;

double current_time = 0.0f;

long ammo_index = 0;
long enemy_index = 0;

float learning_rate = LEARNING_RATE;

int count = 0;
int enemies_spawned = 0;
int enemies_hit = 0;

struct Enemy_Node {
    struct Enemy_Node *next;
    Enemy* enemy;
};

struct Enemy_Collections {
    Enemy_Node* head;
    Enemy_Node* tail;
    int count;
};