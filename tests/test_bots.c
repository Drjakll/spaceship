#include "spaceship_bots.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#c); exit(1); } } while(0)
int main(void) {
    SpaceWorld a, b; SpaceConfig c = space_default_config();
    CHECK(space_init(&a,c,73) && space_init(&b,c,73));
    uint32_t ra=123, rb=123;
    for (int t=0;t<1000;++t) {
        SpaceAction aa[8]={{0}}, bb[8]={{0}};
        space_random_actions(&a,&ra,aa); space_random_actions(&b,&rb,bb);
        CHECK(memcmp(aa,bb,sizeof(aa))==0);
        for(int i=0;i<3;++i) CHECK(aa[i].move>=0 && aa[i].move<=8 && aa[i].fire>=0 && aa[i].fire<=1);
        CHECK(space_step(&a,aa) && space_step(&b,bb));
        CHECK(memcmp(&a,&b,sizeof(a))==0);
        if(a.terminal) {space_reset(&a,73+t);space_reset(&b,73+t);}
    }
    puts("PASS seeded random baseline reproduces actions and complete traces");
    space_reset(&a,73);
    a.enemies[0] = (SpaceEnemy){.phase=1,.x=400,.y=500,.health=10};
    a.enemies[1] = (SpaceEnemy){.phase=1,.x=900,.y=200,.health=10};
    a.enemies[2] = (SpaceEnemy){.phase=2,.x=250,.y=700,.health=0};
    CHECK(space_greedy_target(&a,0)==0 && space_greedy_target(&a,2)==0);
    SpaceAction actions[8]; space_greedy_actions(&a,actions);
    CHECK(actions[0].move==SPACE_DOWN_RIGHT && actions[2].move==SPACE_DOWN_LEFT);
    CHECK(actions[0].fire==1 && actions[2].fire==1);
    a.ships[0].health=0; space_greedy_actions(&a,actions);
    CHECK(actions[0].move==SPACE_STAY && actions[0].fire==0);
    puts("PASS independent greedy agents pursue the most advanced live threat");
    space_reset(&a,73);
    a.enemies[0] = (SpaceEnemy){.phase=1,.x=100,.y=500,.health=10};
    a.enemies[1] = (SpaceEnemy){.phase=1,.x=500,.y=400,.health=10};
    a.enemies[2] = (SpaceEnemy){.phase=1,.x=900,.y=300,.health=10};
    for(int i=0;i<3;++i) CHECK(space_lane_target(&a,i)==i);
    space_lane_actions(&a,actions);
    CHECK(actions[0].move==SPACE_DOWN_LEFT && actions[2].move==SPACE_DOWN_RIGHT);
    a.ships[0].health=0;
    CHECK(space_lane_target(&a,1)==0 && space_lane_target(&a,2)==1);
    SpaceWorld snapshot = a; space_lane_actions(&a,actions);
    CHECK(memcmp(&snapshot,&a,sizeof(a))==0);
    puts("PASS lane team splits threats and redistributes coverage after an ally dies");
    return 0;
}
