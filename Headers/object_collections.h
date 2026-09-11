struct Enemy_Node {
    struct Enemy_Node *next;
    Enemy* enemy;
};

struct Projectile_Node {
    struct Projectile_Node *next;
    Ammo* ammo;
};

struct Enemy_Collections {
    Enemy_Node* head;
    Enemy_Node* tail;

};

struct Projectile_Collections{
    Projectile_Node* head;
    Projectile_Node* tail;
};



void Insert_Enemy(Enemy* enemy){

    Enemy_Node* new_node = calloc(1, sizeof(Enemy_Node));
    new_node->enemy = enemy;

    if(!enemies->head){
        enemies->head = new_node;
        enemies->tail = enemies->head;
        return;
    }

    enemies->tail->next = new_node;

    enemies->tail = new_node;
}

bool Iterate_Enemies(bool (*callback)(Enemy*, double), double deltaTime){

    bool collision_detected = false;

    if(enemies->head){

        if(callback(enemies->head->enemy, deltaTime)){
            collision_detected = true;
        }

        if(enemies->head->enemy->position.y > WINDOW_HEIGHT){

            Enemy *e = enemies->head->enemy;

            free(e->velocity);
            free(e);

            Enemy_Node *temp = enemies->head;

            enemies->head = enemies->head->next;

            free(temp);
        }

    } else {

        return false;

    }

    if(!enemies->head){
        return collision_detected;
    }

    Enemy_Node* ptr = enemies->head;

    while(ptr && ptr->next){

        if(callback(ptr->next->enemy, deltaTime)){
            collision_detected = true;
        }

        if(ptr->next->enemy->position.y > WINDOW_HEIGHT){
        
            Enemy *e = ptr->next->enemy;

            free(e->velocity);
            free(e);

            Enemy_Node *temp = ptr->next;

            if(ptr->next == enemies->tail){
                enemies->tail = ptr;
            }

            ptr->next = ptr->next->next;

            free(temp);

            continue;
        }

        ptr = ptr->next;

    }

    return collision_detected;

}

void Insert_Projectile(Ammo* ammo){

    Projectile_Node* new_node = calloc(1, sizeof(Projectile_Node));
    new_node->ammo = ammo;

    if(!projectiles->head){
        projectiles->head = new_node;
        projectiles->tail = projectiles->head;
        return;
    }

    projectiles->tail->next = new_node;

    projectiles->tail = new_node;
}

bool Iterate_Projectiles(bool (*callback)(Ammo*, double), double deltaTime){

    bool collision_existed = false;

    if(projectiles->head){

        bool collided = callback(projectiles->head->ammo, deltaTime);

        if(collided){
            collision_existed = true;
        }

        if(collided || projectiles->head->ammo->position.y < -MISSILE_RADIUS){

            Projectile_Node *temp = projectiles->head;

            Ammo *a = temp->ammo;

            free(a->velocity);
            free(a);

            projectiles->head = projectiles->head->next;

            free(temp);
        }

        if(!projectiles->head){
            return collision_existed;
        }

    } else {
        
        return collision_existed;

    }

    if(!projectiles->head){
        return collision_existed;
    }
    
    Projectile_Node* ptr = projectiles->head;

    while(ptr && ptr->next){

        bool collided = callback(ptr->next->ammo, deltaTime);

        if(collided){
            collision_existed = true;
        }

        if(collided || ptr->next->ammo->position.y < -MISSILE_RADIUS){

            Projectile_Node *temp = ptr->next;
            Ammo *a = temp->ammo;

            free(a->velocity);
            free(a);

            if(ptr->next == projectiles->tail){
                projectiles->tail = ptr;
            }

            ptr->next = ptr->next->next;

            free(temp);
            continue;
        }

        ptr = ptr->next;

    }

    return collision_existed;

}