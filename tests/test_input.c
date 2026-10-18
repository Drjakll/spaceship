#include "spaceship_input.h"
#include "spaceship_bots.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    SpaceAction action=space_manual_action(true,false,true,false,true);
    assert(action.move==SPACE_UP_LEFT && action.fire==1);
    action=space_manual_action(true,true,true,true,false);
    assert(action.move==SPACE_STAY && action.fire==0);
    SpaceWorld world;assert(space_init(&world,space_default_config(),1000));
    for(int step=0;step<15;++step) {
        SpaceAction actions[8];space_lane_actions(&world,actions);
        actions[0]=space_manual_action(true,false,false,false,true);
        assert(space_step(&world,actions));
    }
    assert(world.ships[0].y==550 && world.ships[1].y>850);
    int shots[3]={0};
    for(int p=0;p<SPACE_MAX_PROJECTILES;++p) if(world.projectiles[p].active) ++shots[world.projectiles[p].owner];
    assert(shots[0]==2 && shots[1]==2 && shots[2]==2);
    puts("PASS keyboard directions with scripted allies and independent cooldown-limited fire");
    return 0;
}
