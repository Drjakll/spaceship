#pragma once
/* Used only by the explicit GPU diagnostic. Normal runs use period=0. */
static void space_diagnostic_fixture(SpaceWorld *world, int step, int period) {
    memset(world->enemies,0,sizeof(world->enemies));
    memset(world->projectiles,0,sizeof(world->projectiles));
    for(int a=0;a<world->config.num_agents;++a) {
        world->ships[a].x=1000.0f*(a+1)/(world->config.num_agents+1);
        world->ships[a].y=850;
    }
    if(step%67==63) {
        world->enemies[0]=(SpaceEnemy){.phase=1,.type=0,.x=100,.y=100,.health=10};
        world->projectiles[0]=(SpaceProjectile){1,0,100,100,0};
        ++world->spawned;
    }
    if(step%67==64) {
        world->enemies[1]=(SpaceEnemy){.phase=1,.type=0,.x=100,.y=1026,.health=10};
        ++world->spawned;
    }
    for(int a=0;a<world->config.num_agents;++a) {
        if(world->ships[a].health>0 && (step%period==0 ||
                (step%67==65 && a==0 && world->config.num_agents>1))) {
            world->ships[a].health=1;
            world->enemies[a+2]=(SpaceEnemy){.phase=2,.x=world->ships[a].x,.y=850,.explosion_ticks=100};
        }
    }
}
