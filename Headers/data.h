//Enemy Data template
struct Enemy_Data {
    Vector2 position;
    float health;
    Vector2 velocity;
    float size_radius;
    char* name;
    bool dead;

};

struct Enemy_Data_Node {
    struct Enemy_Data_Node *next;
    Enemy_Data enemy_data;

};

struct Enemy_Data_List{
    Enemy_Data_Node *head;
    Enemy_Data_Node *tail;

};

char* Add_More_Chars(char *str, char* accum_str){
    int add_length = strlen(str);
    int old_length = strlen(accum_str);

    int new_length = add_length + old_length + 1;

    char *temp = realloc(accum_str, new_length);

    if(!temp){
        return NULL;
    }

    accum_str = temp;

    strcat(accum_str, str);

    return accum_str;

}

void Add_To_Enemy_Data_List(Enemy_Data data){

    Enemy_Data_Node *new_node = calloc(1, sizeof(Enemy_Data_Node));
    new_node->enemy_data = data;

    Enemy_Data_List *list = current_enemy_data_list;

    if(!list->head){
        
        list->head = new_node;
        list->tail = list->head;
        return;

    }

    list->tail->next = new_node;
    list->tail = list->tail->next;

}

bool Add_Enemy_Data_Wrapper(Enemy *enemy, double deltaTime){

    Enemy_Data data = {
        .position = enemy->position,
        .health = enemy->health,
        .velocity = *enemy->velocity,
        .size_radius = enemy->size_r,
        .dead = enemy->dead,
        .name = enemy->name
    };

    Add_To_Enemy_Data_List(data);

    return false;
}

char* Iterate_Enemy_Data_List(Enemy_Data_List list, char* (*callback)(Enemy_Data)){

    Enemy_Data_Node *ptr = list.head;

    char* accum_str = calloc(1, 1);

    while(ptr){

        char* str = callback(ptr->enemy_data);
        accum_str = Add_More_Chars(str, accum_str);

        free(str);

        ptr = ptr->next;
    }

    return accum_str;
}

//Projectile Data template
struct Projectile_Data{
    Vector2 position;
    float damage;
    Vector2 velocity;
    float size_radius;

};

struct Projectile_Data_Node {
    struct Projectile_Data_Node *next;
    Projectile_Data projectile_data;

};

struct Projectile_Data_List{
    Projectile_Data_Node *head;
    Projectile_Data_Node *tail;

};


void Add_To_Projectile_Data_List(Projectile_Data data){

    Projectile_Data_Node *new_node = calloc(1, sizeof(Projectile_Data_Node));
    new_node->projectile_data = data;

    Projectile_Data_List *list = current_projectile_data_list;

    if(!list->head){
        
        list->head = new_node;
        list->tail = list->head;
        return;

    }

    list->tail->next = new_node;
    list->tail = list->tail->next;

}

bool Add_Projectile_Data_Wrapper(Ammo *ammo, double deltaTime){

    Projectile_Data data = {
        .position = ammo->position,
        .damage = ammo->damage,
        .velocity = *ammo->velocity,
        .size_radius = ammo->size_r
    };

    Add_To_Projectile_Data_List(data);

    return false;
}

char* Iterate_Projectile_Data_List(Projectile_Data_List list, char* (*callback)(Projectile_Data)){

    Projectile_Data_Node *ptr = list.head;

    char* accum_str = calloc(1, sizeof(char));

    while(ptr){

        char* str = callback(ptr->projectile_data);
        accum_str = Add_More_Chars(str, accum_str);

        free(str);

        ptr = ptr->next;
    }

    return accum_str;
}


//Spaceship data template
struct Spaceship_Data {
    Vector2 position;
    float health;
    float size_radius;
    float current_weapon_cd;

};


//Overall data template
struct Data{
    int keys_down;
    int current_time_ms;
    int frame_number;
    int current_score;
    Spaceship_Data spaceship_data;
    Enemy_Data_List enemy_data_list;
    Projectile_Data_List projectile_data_list;
    
};

struct Data_Node {
    struct Data_Node *next;
    Data data;
    
};

struct Data_List {
    Data_Node *head;
    Data_Node *tail;
};



void Add_To_Data(Data data, Data_List *list){

    Data_Node *new_node = calloc(1, sizeof(Data_Node));
    new_node->data = data;

    if(!list->head){
        list->head = new_node;
        list->tail = list->head;
        return;
    }

    list->tail->next = new_node;
    list->tail = list->tail->next;
}

char* Iterate_Data(Data_List list, char* (*callback)(Data)){

    Data_Node *ptr = list.head;

    char* accum_str = calloc(1, sizeof(char));

    while(ptr){

        char* str = callback(ptr->data);

        accum_str = Add_More_Chars(str, accum_str);

        free(str);
        
        ptr = ptr->next;
    }

    size_t size = strlen(accum_str) + 3;

    char* final_str = malloc(size);

    size_t index = strlen(accum_str);

    if(index > 2){
        accum_str[index - 2] = '\0';
    }

    snprintf(final_str, size, "[%s]", accum_str);

    free(accum_str);

    return final_str;
}

char* Extract_Enemy_Data_To_Str_JSON(Enemy_Data data){

    char* text = malloc(300);

    snprintf(text, 300,
                    "\t\t{\n"
                        "\t\t\t\"health\": %f,\n"
                        "\t\t\t\"size\": %f,\n"
                        "\t\t\t\"dead\": %d,\n"
                        "\t\t\t\"name\": \"%s\",\n"
                        "\t\t\t\"velocity\": {\n"
                            "\t\t\t\t\"x\": %f,\n"
                            "\t\t\t\t\"y\": %f\n"
                        "\t\t\t},\n"
                        "\t\t\t\"position\": {\n"
                            "\t\t\t\t\"x\": %f,\n"
                            "\t\t\t\t\"y\": %f\n"
                        "\t\t\t}\n"
                    "\t\t},\n",
        data.health,
        data.size_radius,
        data.dead,
        data.name,
        data.velocity.x,
        data.velocity.y,
        data.position.x,
        data.position.y
    );

    return text;
}

char* Extract_Projectile_Data_To_Str_JSON(Projectile_Data data){

    char* text = malloc(200);

    snprintf(text, 200, 
                    "\t\t{\n"
                        "\t\t\t\"damage\": %f,\n"
                        "\t\t\t\"size\": %f,\n"
                        "\t\t\t\"velocity\": {\n"
                            "\t\t\t\t\"x\": %f,\n"
                            "\t\t\t\t\"y\": %f\n"
                        "\t\t\t},\n"
                        "\t\t\t\"position\": {\n"
                            "\t\t\t\t\"x\": %f,\n"
                            "\t\t\t\t\"y\": %f\n"
                        "\t\t\t}\n"
                    "\t\t},\n",
        data.damage,
        data.size_radius,
        data.velocity.x,
        data.velocity.y,
        data.position.x,
        data.position.y
    );

    return text;
}

char* Convert_Data_To_String(Data data){

    int key_strokes = data.keys_down;
    int time = data.current_time_ms;
    int frame_number = data.frame_number;
    int current_score = data.current_score;

    Spaceship_Data spaceship_data = data.spaceship_data;

    float spaceship_health = spaceship_data.health;
    float spaceship_radius = spaceship_data.size_radius;
    float spaceship_weapon_cd = spaceship_data.current_weapon_cd;
    Vector2 spaceship_location = spaceship_data.position;

    Enemy_Data_List enemy_data_list = data.enemy_data_list;
    Projectile_Data_List projectile_data_list = data.projectile_data_list;

    char* enemy_array = Iterate_Enemy_Data_List(enemy_data_list, Extract_Enemy_Data_To_Str_JSON);

    size_t index = strlen(enemy_array);

    if(index > 2){
        enemy_array[index - 2] = '\0';
    }

    char* projectile_array = Iterate_Projectile_Data_List(projectile_data_list, Extract_Projectile_Data_To_Str_JSON);

    index = strlen(projectile_array);

    if(index > 2){
        projectile_array[index - 2] = '\0';
    }

    int needed = snprintf(
                        NULL,
                        0,
                        "{\n"
                            "\t\"key_strokes\": %d,\n" 
                            "\t\"timestamp\": %d,\n" 
                            "\t\"score\": %d,\n"
                            "\t\"frame_number\": %d,\n"
                            "\t\"spaceship\": {\n"
                                "\t\t\"health\": %f,\n"
                                "\t\t\"size\": %f,\n"
                                "\t\t\"weapon_cd\": %f,\n"
                                "\t\t\"position\": {\n"
                                    "\t\t\t\"x\": %f,\n"
                                    "\t\t\t\"y\": %f\n"
                                "\t\t}\n"
                            "\t},\n"
                            "\t\"enemies\": [\n"
                                "%s\n"
                            "\t],\n"
                            "\t\"projectiles\": [\n"
                                "%s\n"
                            "\t]\n"
                        "},\n", 
                            key_strokes, 
                            time,
                            current_score,
                            frame_number,
                            spaceship_health,
                            spaceship_radius,
                            spaceship_weapon_cd,
                            spaceship_location.x,
                            spaceship_location.y,
                            enemy_array,
                            projectile_array
                    );

    char* text = malloc(needed + 1);

    snprintf(text, needed + 1, 
                                "{\n"
                                    "\t\"key_strokes\": %d,\n" 
                                    "\t\"timestamp\": %d,\n" 
                                    "\t\"score\": %d,\n"
                                    "\t\"frame_number\": %d,\n"
                                    "\t\"spaceship\": {\n"
                                        "\t\t\"health\": %f,\n"
                                        "\t\t\"size\": %f,\n"
                                        "\t\t\"weapon_cd\": %f,\n"
                                        "\t\t\"position\": {\n"
                                            "\t\t\t\"x\": %f,\n"
                                            "\t\t\t\"y\": %f\n"
                                        "\t\t}\n"
                                   "\t},\n"
                                   "\t\"enemies\": [\n"
                                        "%s\n"
                                    "\t],\n"
                                   "\t\"projectiles\": [\n"
                                        "%s\n"
                                   "\t]\n"
                                "},\n", 
                                    key_strokes, 
                                    time,
                                    current_score,
                                    frame_number,
                                    spaceship_health,
                                    spaceship_radius,
                                    spaceship_weapon_cd,
                                    spaceship_location.x,
                                    spaceship_location.y,
                                    enemy_array,
                                    projectile_array
                                );

    free(projectile_array);
    free(enemy_array);

    return text;

}