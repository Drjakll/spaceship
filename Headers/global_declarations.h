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
typedef struct Spaceship_Control_Probability;

int score[NUM_OF_AGENTS] = {0};
int frame_count = 0;

Enemy_Collections *enemies = NULL;
Projectile_Collections *projectiles = NULL;
Enemy_Data_List *current_enemy_data_list = NULL;

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

Projectile_Data_List *current_projectile_data_list = NULL;

long ammo_index = 0;