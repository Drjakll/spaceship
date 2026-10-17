long Generate_Enemy_Index() {

    return ++enemy_index;
}


struct Projectile_Node {
    struct Projectile_Node *next;
    Ammo* ammo;
};

struct Projectile_Collections{
    Projectile_Node* head;
    Projectile_Node* tail;
};



void Insert_Enemy(Enemy* enemy, Enemy_Collections* enemies){

    if(!enemy->id){
        enemy->id = Generate_Enemy_Index();
    }
    
    Enemy_Node* new_node = calloc(1, sizeof(Enemy_Node));
    new_node->enemy = enemy;

    enemies->count++;

    if(!enemies->head){
        enemies->head = new_node;
        enemies->tail = enemies->head;
        return;
    }

    enemies->tail->next = new_node;

    enemies->tail = new_node;
}

bool Iterate_Enemies(bool (*callback)(Enemy*, double), double deltaTime, Enemy_Collections* enemies){

    bool collision_detected = false;

    while(enemies->head){

        Enemy_Node *temp = enemies->head;
        Enemy *e = temp->enemy;

        bool collided = callback(e, deltaTime);
        collision_detected |= collided;

        if(e->position.y <= WINDOW_HEIGHT){
            break;
        }

        enemies->head = temp->next;
        enemies->count--;

        free(e->velocity);
        free(e->time_velocity_shifted);
        free(e);
        free(temp);

    } 

    if(!enemies->head){
        enemies->tail = NULL;
        return collision_detected;
    }

    Enemy_Node* ptr = enemies->head;

    while(ptr && ptr->next){

        if(callback(ptr->next->enemy, deltaTime)){
            collision_detected = true;
        }

        if(ptr->next->enemy->position.y > WINDOW_HEIGHT){

            enemies->count--;
        
            Enemy *e = ptr->next->enemy;

            free(e->velocity);
            free(e->time_velocity_shifted);
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

    while (projectiles->head) {
        Projectile_Node *temp = projectiles->head;
        Ammo *a = temp->ammo;

        bool collided = callback(a, deltaTime);
        collision_existed |= collided;

        if (!collided && a->position.y >= -MISSILE_RADIUS) {
            break;
        }

        if (!collided) {
            Learn_To_Shoot(a->locked_target_id, a, false);
        }

        projectiles->head = temp->next;
        free(a->velocity);
        free(a);
        free(temp);
    }


    if(!projectiles->head){
        projectiles->tail = NULL;
        return collision_existed;
    }
    
    Projectile_Node* ptr = projectiles->head;

    while(ptr && ptr->next){

        bool collided = callback(ptr->next->ammo, deltaTime);

        if(collided){
            collision_existed = true;
        }

        if(collided || ptr->next->ammo->position.y < -MISSILE_RADIUS){

            if(!collided){
                Learn_To_Shoot(ptr->next->ammo->locked_target_id, ptr->next->ammo, false);
            }

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