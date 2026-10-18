#include "spaceship_view.h"
#include "spaceship_bots.h"
#include "spaceship_eval.h"
#include "spaceship_input.h"
#include <string.h>
#include <math.h>
#ifdef PUFFERCPU_SOURCE
#define SPACE_POLICY_LIBRARY
#include "checkpoint.c"
#endif
int main(int argc,char **argv) {
#ifdef SPACE_ASSET_ROOT
    if(!ChangeDirectory(SPACE_ASSET_ROOT)) {fputs("Cannot find game assets\n",stderr);return 2;}
#endif
    SpaceConfig config=space_default_config();uint32_t seed=1000,bot_rng=73;
    const char *checkpoint=NULL,*policy="lanes",*trace_path=NULL;int warmup=0,frames=0,manual=-1;
    for(int i=1;i<argc;++i) {
        if(i+1==argc) {fputs("Every option needs a value\n",stderr);return 2;}
        if(!strcmp(argv[i],"--ships")) config.num_agents=space_parse_int(argv[++i],1,8);
        else if(!strcmp(argv[i],"--seed")) seed=(uint32_t)space_parse_int(argv[++i],0,INT_MAX);
        else if(!strcmp(argv[i],"--policy")) policy=argv[++i];
        else if(!strcmp(argv[i],"--checkpoint")) checkpoint=argv[++i];
        else if(!strcmp(argv[i],"--warmup")) warmup=space_parse_int(argv[++i],0,100000);
        else if(!strcmp(argv[i],"--frames")) frames=space_parse_int(argv[++i],0,100000);
        else if(!strcmp(argv[i],"--manual")) manual=space_parse_int(argv[++i],1,8)-1;
        else if(!strcmp(argv[i],"--trace")) trace_path=argv[++i];
        else {fprintf(stderr,"Unknown option %s\n",argv[i]);return 2;}
    }
    if(strcmp(policy,"lanes") && strcmp(policy,"greedy") && strcmp(policy,"random")) return 2;
    if(manual>=config.num_agents || (manual>=0 && checkpoint)) {fputs("Manual mode requires a living team slot and scripted allies\n",stderr);return 2;}
#ifdef PUFFERCPU_SOURCE
    SpacePolicy neural={0};
    if(checkpoint) {int sizes[]={9,2};neural.weights=checked_weights(checkpoint);
        neural.net=make_puffernet(neural.weights,config.num_agents,SPACE_OBSERVATION_SIZE,64,2,sizes,2);srand(seed^0x85ebca6bu);}
#else
    if(checkpoint) {fputs("Rebuild with --pufferlib-root for checkpoint viewing\n",stderr);return 2;}
#endif
    SpaceWorld world;if(!space_init(&world,config,seed)) return 2;
    for(int i=0;i<warmup && !world.terminal;++i) {SpaceAction actions[8];space_lane_actions(&world,actions);if(!space_step(&world,actions)) return 2;}
    SpaceView view={0};if(!space_view_open(&view)) return 2;
    int result=0;
    FILE *trace=trace_path?fopen(trace_path,"wx"):NULL;
    if(trace_path && !trace) {perror("Create trace");result=2;goto finish;}
    bool paused=false;double accumulated=0;int frame=0;
    while(!WindowShouldClose() && (!frames || frame<frames)) {
        if(IsKeyPressed(KEY_P)) paused=!paused;
        if(IsKeyPressed(KEY_R)) {space_reset(&world,++seed);accumulated=0;
#ifdef PUFFERCPU_SOURCE
            if(neural.net) memset(neural.net->mingru->state,0,2*config.num_agents*64*sizeof(float));
#endif
        }
        if(!paused && !world.terminal) accumulated+=fmin(GetFrameTime(),.25);
        while(accumulated>=config.frame_skip/120.0 && !world.terminal && !paused) {
            SpaceAction actions[8]={{0}};
#ifdef PUFFERCPU_SOURCE
            if(neural.net) policy_action(&world,actions,&neural);else
#endif
            if(!strcmp(policy,"random")) space_random_actions(&world,&bot_rng,actions);
            else if(!strcmp(policy,"greedy")) space_greedy_actions(&world,actions);else space_lane_actions(&world,actions);
            if(manual>=0) actions[manual]=space_manual_action(IsKeyDown(KEY_W)||IsKeyDown(KEY_UP),
                IsKeyDown(KEY_S)||IsKeyDown(KEY_DOWN),IsKeyDown(KEY_A)||IsKeyDown(KEY_LEFT),
                IsKeyDown(KEY_D)||IsKeyDown(KEY_RIGHT),IsKeyDown(KEY_SPACE));
            if(!space_step(&world,actions)) {result=2;goto finish;}
            if(trace && manual>=0) {
                int shots[8]={0};for(int p=0;p<SPACE_MAX_PROJECTILES;++p) if(world.projectiles[p].active) ++shots[world.projectiles[p].owner];
                fprintf(trace,"{\"tick\":%d,\"move\":%d,\"fire\":%d,\"x\":%.3f,\"y\":%.3f,\"manual_shots\":%d,\"ally_shots\":%d}\n",
                    world.tick,actions[manual].move,actions[manual].fire,world.ships[manual].x,world.ships[manual].y,shots[manual],shots[(manual+1)%config.num_agents]);
                fflush(trace);
            }
            accumulated-=config.frame_skip/120.0;
        }
        const char *label=checkpoint?"CHECKPOINT / SEE RUN MANIFEST":!strcmp(policy,"lanes")?"SCRIPTED / LANE COVERAGE":
            !strcmp(policy,"greedy")?"SCRIPTED / INDEPENDENT GREEDY":"SCRIPTED / RANDOM ACTIONS";
        if(manual>=0) label="MANUAL + SCRIPTED ALLIES";
        space_view_draw(&view,&world,label,paused);++frame;
    }
finish:
    space_view_close(&view);
    if(trace) fclose(trace);
#ifdef PUFFERCPU_SOURCE
    if(neural.net) {free_puffernet(neural.net);free(neural.weights);}
#endif
    return result;
}
