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
    return 0;
}
