#include "spaceship_view.h"
#include "spaceship_bots.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
int main(void) {
    SpaceWorld world; assert(space_init(&world,space_default_config(),1000));
    for(int i=0;i<900;++i) {SpaceAction actions[8];space_lane_actions(&world,actions);assert(space_step(&world,actions));}
    SpaceWorld before=world, reference=world; SpaceView view={0};
    assert(space_view_open(&view));
    for(int i=0;i<60;++i) {
        space_view_draw(&view,&world,"SCRIPTED / LANE COVERAGE",true);
        assert(!memcmp(&world,&before,sizeof(world)));
    }
    Image capture=LoadImageFromScreen();
    assert(ExportImage(capture,"build/viewer-smoke.png"));
    UnloadImage(capture);
    SpaceAction actions[8];space_lane_actions(&world,actions);
    assert(space_step(&world,actions) && space_step(&reference,actions));
    assert(!memcmp(&world,&reference,sizeof(world)));
    space_view_close(&view);
    puts("PASS rendered arena state and subsequent trajectory match headless simulation");
    return 0;
}
