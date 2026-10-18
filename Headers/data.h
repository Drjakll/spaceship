//Enemy Data template
struct Enemy_Data {
    Vector2 position;
    float health;
    Vector2 velocity;
    float size_radius;
    char* name;
    bool dead;
    int id;
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
        .name = enemy->name,
        .id = enemy->id
    };

    Add_To_Enemy_Data_List(data);

    return false;
}

Enemy_Data* Find_Target_Enemy_Data(Enemy_Data_List list, long id){

    Enemy_Data_Node *ptr = list.head;

    while(ptr){

        if(ptr->enemy_data.id == id){
            return &ptr->enemy_data;
        }

        ptr = ptr->next;
    }

    return NULL;
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
    Spaceship_State state;
};


//Overall data template
struct Data{
    int keys_down;
    int all_keysdown[NUM_OF_AGENTS];
    int current_time_ms;
    int frame_number;
    int* current_scores;
    Spaceship_Data* spaceship_data;
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
    int count;
};

void Append_To_Data(Data data, Data_List *list){

    if(!list){
        return;
    }

    Data_Node *new_node = calloc(1, sizeof(Data_Node));

    if(!new_node){
        return;
    }

    list->count++;

    new_node->data = data;

    if(list->count > MAX_DATA_STORAGE && list->head != NULL){

        Data_Node* temp = list->head;

        list->head = list->head->next;

        list->count--;

        Enemy_Data_Node *enemy = temp->data.enemy_data_list.head;

        while (enemy) {
            Enemy_Data_Node *next = enemy->next;
            free(enemy);
            enemy = next;
        }

        free(temp->data.spaceship_data);
        free(temp);
    }

    if(list->head == NULL){

        list->head = new_node;

        list->tail = new_node;

        return;
    }

    list->tail->next = new_node;
    list->tail = new_node;
}

void Add_To_Data(Data data, Data_List *list){

    if(!list->count){
        list->count = 0;
    }

    list->count++;

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


/*
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
    int current_score = data.current_scores;

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
*/

void Create_Snapshot_Of_Enemies_For_Spaceship(Spaceship *spaceship, int* keysdown, double delta_time){

    Enemy_Data_List *enemies_data = calloc(1, sizeof(Enemy_Data_List));

    current_enemy_data_list = enemies_data;

    Iterate_Enemies(Add_Enemy_Data_Wrapper, delta_time, enemy_collections);

    Spaceship_Data* spaceship_data_entry = calloc(1, sizeof(Spaceship_Data));

    spaceship_data_entry->position = spaceship->position;
    spaceship_data_entry->health = spaceship->health;
    spaceship_data_entry->size_radius = spaceship->size_r;
    spaceship_data_entry->current_weapon_cd = spaceship->weapon->cd_remain;
    spaceship_data_entry->state = spaceship->state;

    Data* data_entry = calloc(1, sizeof(Data));

    *data_entry = (Data){
        .keys_down = *keysdown,
        .current_time_ms = (int)(current_time * 1000.0),
        .spaceship_data = spaceship_data_entry,
        .enemy_data_list = *enemies_data,
        .projectile_data_list = {0},
        .frame_number = 0,
        .current_scores = 0
    };

    Append_To_Data(*data_entry, spaceship->snapshots);

    free(data_entry);
    free(enemies_data);
}

Data* Create_Snapshot(Spaceship *spaceships[], int frame_counts, int scores[], double delta_time, int* keysdown){

    Enemy_Data_List *enemies_data = calloc(1, sizeof(Enemy_Data_List));
    Projectile_Data_List *projectiles_data = calloc(1, sizeof(Projectile_Data_List));



    current_enemy_data_list = enemies_data;

    current_projectile_data_list = projectiles_data;

    Iterate_Enemies(Add_Enemy_Data_Wrapper, delta_time, enemy_collections);
    Iterate_Projectiles(Add_Projectile_Data_Wrapper, delta_time);

    Spaceship_Data* spaceship_data_entries = calloc(NUM_OF_AGENTS, sizeof(Spaceship_Data));

    for(int i = 0 ; i < NUM_OF_AGENTS; i++){

        Spaceship_Data entry = {
            .position = spaceships[i]->position,
            .health = spaceships[i]->health,
            .size_radius = spaceships[i]->size_r,
            .current_weapon_cd = spaceships[i]->weapon->cd_remain,
            .state = spaceships[i]->state
        };

        spaceship_data_entries[i] = entry;
    }

    Data* data_entry = calloc(1, sizeof(Data));
    
    *data_entry = (Data){
        .keys_down = 0,
        .current_time_ms = (int)(current_time * 1000.0),
        .spaceship_data = spaceship_data_entries,
        .enemy_data_list = *enemies_data,
        .projectile_data_list = *projectiles_data,
        .frame_number = frame_counts,
        .current_scores = scores
    };

    for(int i = 0; i < NUM_OF_AGENTS; i++){
        data_entry->all_keysdown[i] = keysdown[i];
    }

    return data_entry;
}

cJSON* Generate_Entry(int x, int y, Reg_Data* rd, int weight_size){
    cJSON *json = cJSON_CreateObject();
    cJSON *array = cJSON_CreateArray();

    cJSON_AddItemToObject(json, "x", cJSON_CreateNumber(x));
    cJSON_AddItemToObject(json, "y", cJSON_CreateNumber(y));
    cJSON_AddItemToObject(json, "bias", cJSON_CreateNumber(rd->bias));
    cJSON_AddItemToObject(json, "z", cJSON_CreateNumber(rd->z));
    cJSON_AddItemToObject(json, "exp_e", cJSON_CreateNumber(rd->exp_e));
    cJSON_AddItemToObject(json, "probability", cJSON_CreateNumber(rd->probability));

    for(int i = 0; i < weight_size; i++){
        cJSON_AddItemToArray(array, cJSON_CreateNumber(rd->weight[i]));
    }

    cJSON_AddItemToObject(json, "weight", array);

    return json;

}

cJSON* Generate_Entry1D(int x, Reg_Data* rd, int weight_size){
    cJSON *json = cJSON_CreateObject();
    cJSON *array = cJSON_CreateArray();

    cJSON_AddItemToObject(json, "dist", cJSON_CreateNumber(x));
    cJSON_AddItemToObject(json, "bias", cJSON_CreateNumber(rd->bias));
    cJSON_AddItemToObject(json, "z", cJSON_CreateNumber(rd->z));
    cJSON_AddItemToObject(json, "exp_e", cJSON_CreateNumber(rd->exp_e));
    cJSON_AddItemToObject(json, "probability", cJSON_CreateNumber(rd->probability));

    for(int i = 0; i < weight_size; i++){
        cJSON_AddItemToArray(array, cJSON_CreateNumber(rd->weight[i]));
    }

    cJSON_AddItemToObject(json, "weight", array);

    return json;
}

void Save(Coordinate_Probability* cp, int i){

    int x = 2 * WINDOW_WIDTH / MAP_FACTOR;
    int y = 2 * WINDOW_HEIGHT / MAP_FACTOR;

    
    cJSON *shoot = cJSON_CreateArray();
    cJSON *move_h = cJSON_CreateArray();
    cJSON *move_v = cJSON_CreateArray();
    cJSON *approach = cJSON_CreateArray();
    cJSON *whole_json = cJSON_CreateObject();

    for(int i = 0; i < x; i++){

        for(int j = 0; j < y; j++){

            for(int k = 0; k < 2; k++){
                cJSON* shoot_json = Generate_Entry(i, j, &cp->shoot_prob[i][j].shoot[k], 2);

                cJSON_AddItemToArray(shoot, shoot_json);
            }

            for(int k = 0; k < 3; k++){
                cJSON* move_h_json = Generate_Entry(i, j, &cp->move_prob_h[i][j].move[k], 3);

                cJSON_AddItemToArray(move_h, move_h_json);

                cJSON* move_v_json = Generate_Entry(i, j, &cp->move_prob_v[i][j].move[k], 3);

                cJSON_AddItemToArray(move_v, move_v_json);
            }
        }
    }

    for(int i = 0; i < distance_size; i++){

        for(int j = 0; j < 2; j++){

            cJSON* to_approach = Generate_Entry1D(i, &cp->approach_prob[i].approach[j], 2);

            cJSON_AddItemToArray(approach, to_approach);

        }
    }

    cJSON_AddItemToObject(whole_json, "shoot", shoot);
    cJSON_AddItemToObject(whole_json, "move_h", move_h);
    cJSON_AddItemToObject(whole_json, "move_v", move_v);
    cJSON_AddItemToObject(whole_json, "approach", approach);

    char *json_str = cJSON_Print(whole_json);

    char filename[64];

    snprintf(filename, sizeof(filename), "Game_Data/trained%i.json", i);

    FILE *file = fopen(filename, "w");

    if (file == NULL) {
        printf("Failed to open file.\n");
        return;
    }

    fputs(json_str, file);

    fclose(file);
}



void Add_Reg_Data(Reg_Data *rd, cJSON* item, int weight_size){

    cJSON* arr = cJSON_GetObjectItemCaseSensitive(item, "weight");
    const char *fields[] = {"bias", "z", "exp_e", "probability"};
    cJSON *values[4];

    if(!rd->weight || !cJSON_IsArray(arr) || cJSON_GetArraySize(arr) != weight_size){
        return;
    }
    for(int i = 0; i < weight_size; i++){
        if(!cJSON_IsNumber(cJSON_GetArrayItem(arr, i))){
            return;
        }
    }
    for(int i = 0; i < 4; i++){
        values[i] = cJSON_GetObjectItemCaseSensitive(item, fields[i]);
        if(!cJSON_IsNumber(values[i])){
            return;
        }
    }
    for(int i = 0; i < weight_size; i++){
        rd->weight[i] = cJSON_GetArrayItem(arr, i)->valuedouble;
    }
    rd->bias = values[0]->valuedouble;
    rd->z = values[1]->valuedouble;
    rd->exp_e = values[2]->valuedouble;
    rd->probability = values[3]->valuedouble;

}

void Read_Data(Coordinate_Probability* cp, int i){

    char filename[64];

    snprintf(filename, sizeof(filename), "Game_Data/trained%i.json", i);

    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        printf("Cannot open file\n");
        return;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    if (size < 0) {
        fclose(file);
        return;
    }

    char *buffer = malloc((size_t)size + 1);

    if (buffer == NULL) {
        fclose(file);
        return;
    }

    size_t bytesRead = fread(buffer, 1, (size_t)size, file);
    fclose(file);

    buffer[bytesRead] = '\0';

    if (bytesRead != (size_t)size) {
        free(buffer);
        return;
    }

    cJSON *json = cJSON_Parse(buffer);

    free(buffer);

    if (json == NULL) {
        printf("Invalid JSON\n");
        return;
    }

    int max_x = 2 * WINDOW_WIDTH / MAP_FACTOR;
    int max_y = 2 * WINDOW_HEIGHT / MAP_FACTOR;
    const char *names[] = {"shoot", "move_h", "move_v", "approach"};

    for(int section = 0; section < 4; section++){
        
        cJSON *array = cJSON_GetObjectItemCaseSensitive(json, names[section]);

        if(!array){
            continue;
        }

        int actions = section == 0 ? 2 : 3;
        actions = section == 3 ? 2 : actions;

        int count = cJSON_GetArraySize(array);

        for(int i = 0; i < count; i++){

            cJSON *item = cJSON_GetArrayItem(array, i);
            cJSON *y_item;
            cJSON *x_item;

            if(section < 3){
                x_item = cJSON_GetObjectItemCaseSensitive(item, "x");
                y_item = cJSON_GetObjectItemCaseSensitive(item, "y");
            }
            else{
                x_item = cJSON_GetObjectItemCaseSensitive(item, "dist");
            }

            if(!cJSON_IsNumber(x_item) || (section < 3 && !cJSON_IsNumber(y_item))){
                continue;
            }
            

            double x_value = x_item->valuedouble;
            double y_value = section < 3 ? y_item->valuedouble : 0 ;
            if(!isfinite(x_value) || !isfinite(y_value) ||
               x_value < 0 || x_value >= (section == 3  ? distance_size : max_x) || y_value < 0 || y_value >= max_y ||
               floor(x_value) != x_value || floor(y_value) != y_value){
                continue;
            }
            int x = (int)x_value;
            int y = (int)y_value;
            int index = i % actions;
            Reg_Data *rd;
            if(section == 0){
                rd = &cp->shoot_prob[x][y].shoot[index];
            } else if(section == 1){
                rd = &cp->move_prob_h[x][y].move[index];
            } else if(section == 2){
                rd = &cp->move_prob_v[x][y].move[index];
            } else {
                rd = &cp->approach_prob[x].approach[index];
            }
            Add_Reg_Data(rd, item, actions);
        }
    }

    cJSON_Delete(json);
}
