#include PUFFERCPU_SOURCE
#include "spaceship_eval.h"
#define SPACE_WEIGHT_COUNT (SPACE_OBSERVATION_SIZE*64 + 12*64 + 2*3*64*64)
typedef struct {
    Weights *weights;
    PufferNet *net;
    float observations[SPACE_MAX_AGENTS*SPACE_OBSERVATION_SIZE];
    float actions[SPACE_MAX_AGENTS*2], terminals[SPACE_MAX_AGENTS];
} SpacePolicy;
static void policy_action(const SpaceWorld *world, SpaceAction *actions, void *context) {
    SpacePolicy *policy=context;
    for(int a=0;a<world->config.num_agents;++a)
        if(!space_observe(world,a,policy->observations+a*SPACE_OBSERVATION_SIZE)) abort();
    forward_puffernet(policy->net,policy->observations,policy->actions,NULL,policy->terminals);
    for(int a=0;a<world->config.num_agents;++a) {
        actions[a].move=(int)policy->actions[2*a]; actions[a].fire=(int)policy->actions[2*a+1];
    }
}
static Weights *checked_weights(const char *path) {
    struct stat status;
    if(stat(path,&status) || status.st_size!=(long)(SPACE_WEIGHT_COUNT*sizeof(float))) {
        fputs("Invalid checkpoint size for Spaceship 2554/64/2/[9,2] model\n",stderr); exit(2);
    }
    Weights *weights=load_weights(path);
    if(!weights) {fputs("Unable to load checkpoint\n",stderr);exit(2);}
    for(int i=0;i<SPACE_WEIGHT_COUNT;++i) if(!isfinite(weights->data[i])) {
        fputs("Non-finite checkpoint weight\n",stderr);exit(2);
    }
    return weights;
}
#ifndef SPACE_POLICY_LIBRARY
int main(int argc, char **argv) {
    if(argc==4 && !strcmp(argv[1],"--init-untrained")) {
        uint32_t rng=(uint32_t)space_parse_int(argv[3],1,INT_MAX);
        Weights *weights=calloc(1,sizeof(Weights)+(SPACE_WEIGHT_COUNT+7)*sizeof(float));
        if(!weights) return 2;
        weights->data=(float*)(weights+1); weights->size=SPACE_WEIGHT_COUNT+7;
        for(int i=0;i<SPACE_WEIGHT_COUNT;++i) {
            rng^=rng<<13; rng^=rng>>17; rng^=rng<<5;
            weights->data[i]=((rng>>8)*(1.0f/16777216.0f)-.5f)*.02f;
        }
        int sizes[]={9,2}; PufferNet *net=make_puffernet(weights,3,SPACE_OBSERVATION_SIZE,64,2,sizes,2);
        if(weights->idx!=SPACE_WEIGHT_COUNT) {fputs("Native model layout mismatch\n",stderr);return 2;}
        FILE *file=fopen(argv[2],"wbx");
        if(!file) {perror("Create untrained checkpoint");return 2;}
        size_t written=fwrite(weights->data,sizeof(float),SPACE_WEIGHT_COUNT,file);
        if(fclose(file) || written!=SPACE_WEIGHT_COUNT) return 2;
        free_puffernet(net);free(weights);
        fputs("Created UNTRAINED initialization; no PPO updates performed\n",stderr);
        return 0;
    }
    if(argc!=5) {fputs("Usage: checkpoint WEIGHTS SHIPS SEED WAVE_SIZE | --init-untrained PATH SEED\n",stderr);return 2;}
    SpaceConfig config=space_default_config(); config.num_agents=space_parse_int(argv[2],1,8);
    uint32_t seed=(uint32_t)space_parse_int(argv[3],0,INT_MAX), action_seed=seed^0x85ebca6bu;
    config.wave_size=space_parse_int(argv[4],1,1000000);
    SpacePolicy policy={0}; policy.weights=checked_weights(argv[1]); int sizes[]={9,2};
    policy.net=make_puffernet(policy.weights,config.num_agents,SPACE_OBSERVATION_SIZE,64,2,sizes,2);
    if(policy.weights->idx!=SPACE_WEIGHT_COUNT) abort();
    srand(action_seed);
    SpaceEpisode episode; space_evaluate(&episode,config,seed,policy_action,&policy);
    space_episode_json(&episode,seed,action_seed);
    free_puffernet(policy.net);free(policy.weights);
    return 0;
}
#endif
