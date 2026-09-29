#include "spaceship_eval.h"
#include "spaceship_bots.h"
#include <string.h>
typedef struct {int kind; uint32_t rng;} Bot;
static void control(const SpaceWorld *world, SpaceAction *actions, void *context) {
    Bot *bot=context;
    if(bot->kind==0) space_random_actions(world,&bot->rng,actions);
    else if(bot->kind==1) space_greedy_actions(world,actions);
    else space_lane_actions(world,actions);
}
int main(int argc, char **argv) {
    if(argc!=5) {fputs("Usage: evaluate random|greedy|lanes SHIPS SEED WAVE_SIZE\n",stderr);return 2;}
    Bot bot={0};
    if(!strcmp(argv[1],"greedy")) bot.kind=1;
    else if(!strcmp(argv[1],"lanes")) bot.kind=2;
    else if(strcmp(argv[1],"random")) {fputs("Unknown policy\n",stderr);return 2;}
    SpaceConfig config=space_default_config();
    config.num_agents=space_parse_int(argv[2],1,8);
    uint32_t seed=(uint32_t)space_parse_int(argv[3],0,INT_MAX);
    config.wave_size=space_parse_int(argv[4],1,1000000);
    uint32_t action_seed=seed^0x85ebca6bu; bot.rng=action_seed;
    SpaceEpisode episode; space_evaluate(&episode,config,seed,control,&bot);
    space_episode_json(&episode,seed,action_seed);
    return 0;
}
