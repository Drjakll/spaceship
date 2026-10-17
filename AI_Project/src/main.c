#include <raylib.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Independent fixed-step environment. Rendering never changes simulation speed. */
#define W 1000
#define H 1000
#define ENEMIES 12
#define BULLETS 32
#define BLASTS 128
#define ACTIONS 9
#define STATES (7*7*3*3*3*3*3)
#define LIMIT 1800
#define START_HEALTH 1000
#define DT (1.0f/30.0f)
typedef struct { float health,damage,acceleration,cooldown; int points; } EnemyStats;
static const EnemyStats enemy_stats[3]={{25,5,90,1,2},{37.5f,7,72,1.2f,4},{50,10,54,1.4f,8}};
#define MODEL "models/zigzag_policy.bin"
typedef struct { float x,y; bool active; int type,ttl; float hp,attack,vy,vx,patrol_left,patrol_right; } Body;
typedef struct {
    float x,y; int health,tick,kills,hits,shots,landed,spawned,cooldown,score; float spawn;
    bool done; Body enemies[ENEMIES], bullets[BULLETS], blasts[BLASTS];
} Game;
typedef struct { float q[STATES][ACTIONS]; uint32_t episodes; } Agent;
static Agent agent;
static uint32_t rng=12345;
static uint32_t random_u(void) { rng ^= rng<<13; rng ^= rng>>17; rng ^= rng<<5; return rng; }
static float random_f(void) { return (random_u()>>8)*(1.0f/16777216.0f); }
static float clamp(float x,float lo,float hi) { return x<lo?lo:x>hi?hi:x; }
static void reset(Game *g) { *g=(Game){.x=W/2,.y=H-90,.health=START_HEALTH}; }
static int bucket(float d) {
    return d < -240 ? 0 : d < -90 ? 1 : d < -25 ? 2 : d < 25 ? 3 : d < 90 ? 4 : d < 240 ? 5 : 6;
}
static int state(const Game *g) {
    float best=1e20f,dx=0,dy=0; bool found=false; int threat=0;
    for(int i=0;i<ENEMIES;i++) if(g->enemies[i].active) {
        float x=g->enemies[i].x-g->x,y=g->enemies[i].y-g->y,d=x*x+y*y;
        if(d<best) { best=d; dx=x; dy=y; found=true; threat=1; }
    }
    /* Explosions have larger collision radii: prioritize their effective distance. */
    for(int i=0;i<BLASTS;i++) if(g->blasts[i].active) {
        float x=g->blasts[i].x-g->x,y=g->blasts[i].y-g->y;
        float surface=fmaxf(0,sqrtf(x*x+y*y)-35),d=surface*surface;
        if(d<best) { best=d; dx=x; dy=y; found=true; threat=2; }
    }
    int bx=(int)(g->x/(W/3.0f)),by=(int)((g->y-60)/((H-80)/3.0f));
    if(bx>2) bx=2;
    if(by>2) by=2;
    int s=bucket(dx)*7+bucket(dy);
    s=s*3+bx; s=s*3+by;
    s=s*3+(found?(best<100*100?1:2):0);
    s=s*3+(g->cooldown==0?0:g->cooldown<5?1:2); return s*3+threat;
}
/* Nine movement actions; the ship auto-fires. Q-learning learns navigation. */
static void explode(Game *g,Body *enemy) {
    int slot=-1;
    for(int i=0;i<BLASTS;i++) if(!g->blasts[i].active) { slot=i; break; }
    assert(slot>=0); /* At most 80 spawns in a 1,800-step episode. */
    g->blasts[slot]=(Body){.x=enemy->x,.y=enemy->y,.active=true,
        .vy=100,.vx=1,.attack=enemy->attack};
    enemy->active=false;
}
static float step(Game *g,int action) {

    if(g->done) return 0;
    assert(action>=0 && action<ACTIONS);
    float reward=0.01f,ox=g->x,oy=g->y;
    int mx=action%3-1,my=action/3-1;
    /* Match the original 400 px/s ship speed; previously only 120 px/s. */
    float speed=400.0f*DT/((mx && my)?sqrtf(2):1);
    g->x=clamp(g->x+mx*speed,25,W-25);
    g->y=clamp(g->y+my*speed,60,H-25);
    if((mx && g->x==ox)||(my && g->y==oy)) reward-=0.03f;
    if(g->cooldown>0) g->cooldown--;
    if(g->cooldown==0) for(int i=0;i<BULLETS;i++) if(!g->bullets[i].active) {
        g->bullets[i]=(Body){.x=g->x,.y=g->y-18,.active=true}; g->cooldown=8; g->shots++; break;
    }
    if((g->spawn-=DT)<=0) {
        for(int i=0;i<ENEMIES;i++) if(!g->enemies[i].active) {
            g->spawned++;
            int type=(int)(random_u()%3);
            g->enemies[i]=(Body){.x=25+random_f()*(W-50),.y=35,.active=true,.type=type,.hp=enemy_stats[type].health,.vy=10,.vx=200};
            Body *e=&g->enemies[i];
            e->patrol_left=fmaxf(25,e->x-175);
            e->patrol_right=fminf(W-25,e->x+175);
            break;
        }
        g->spawn+=0.75f;
    }
    for(int i=0;i<BULLETS;i++) if(g->bullets[i].active) {
        Body *b=&g->bullets[i]; b->y-=12;
        if(b->y<0) b->active=false;
    }
    for(int i=0;i<BLASTS;i++) if(g->blasts[i].active) {
        Body *b=&g->blasts[i]; b->vy-=DT; b->y+=b->vy*DT;
        int band=(int)floorf(b->x)%350;
        if(band>0 && band<10) b->vx=-b->vx;
        b->x+=b->vx*DT;
        if(b->y>H) { b->active=false; continue; }
        if(b->attack>0) b->attack=fmaxf(0,b->attack-DT);
        float dx=g->x-b->x,dy=g->y-b->y;
        if(dx*dx+dy*dy<75*75 && b->attack==0) {
            b->attack=0.15f; g->health--; g->hits++; reward-=5;
            if(g->health<=0) { g->health=0; g->done=true; return reward-10; }
        }
    }
    for(int i=0;i<ENEMIES;i++) if(g->enemies[i].active) {
        Body *e=&g->enemies[i];
        float dx=g->x-e->x,dy=g->y-e->y;
        /* Fixed zigzag patrol: independent of ship and missile positions. */
        e->vy+=enemy_stats[e->type].acceleration*DT;
        e->y+=e->vy*DT;
        e->x+=e->vx*DT;
        if(e->patrol_right>e->patrol_left) {
            if(e->x>=e->patrol_right) {
                e->x=2*e->patrol_right-e->x; e->vx=-fabsf(e->vx);
            } else if(e->x<=e->patrol_left) {
                e->x=2*e->patrol_left-e->x; e->vx=fabsf(e->vx);
            }
        }
        if(e->y>H+30) { e->active=false; reward-=1.0f; continue; }
        if(e->attack>0) e->attack=fmaxf(0,e->attack-DT);

        for(int j=0;j<BULLETS;j++) if(g->bullets[j].active) {
            float bx=e->x-g->bullets[j].x,by=e->y-g->bullets[j].y;
            if(bx*bx+by*by<35*35) {
                g->bullets[j].active=false; g->landed++; reward+=0.15f;
                if((e->hp-=10)<=0) { explode(g,e); g->kills++; g->score+=enemy_stats[e->type].points; reward+=12; break; }
            }
        }
        dx=g->x-e->x; dy=g->y-e->y;
        if(e->active && dx*dx+dy*dy<50*50 && e->attack==0) {
            e->attack=enemy_stats[e->type].cooldown; g->health-=(int)enemy_stats[e->type].damage; g->hits++; reward-=5;
            if(g->health<=0) { reward-=10; g->done=true; break; }
        }
    }
    /* Dense feedback makes near misses matter, even with 1,000 health.
       Include all hazards so escaping one enemy into another is discouraged. */
    float danger=0;
    for(int i=0;i<ENEMIES;i++) if(g->enemies[i].active) {
        Body *e=&g->enemies[i];
        float dx=e->x-g->x,dy=e->y-g->y;
        float clearance=sqrtf(dx*dx+dy*dy)-50;
        danger+=clamp(1-clearance/140,0,1);
    }
    for(int i=0;i<BLASTS;i++) if(g->blasts[i].active) {
        Body *b=&g->blasts[i];
        float dx=b->x-g->x,dy=b->y-g->y;
        float clearance=sqrtf(dx*dx+dy*dy)-75;
        danger+=clamp(1-clearance/140,0,1);
    }
    reward-=0.025f*danger;
    /* Seek a firing lane below a live target, while outside contact range.
       Only the best lane pays, so crowds cannot multiply this bonus. */
    float firing_lane=0;
    if(danger<0.5f) for(int i=0;i<ENEMIES;i++) if(g->enemies[i].active) {
        Body *e=&g->enemies[i];
        float ahead=g->y-e->y;
        if(ahead>100 && ahead<450) {
            float alignment=clamp(1-fabsf(g->x-e->x)/120,0,1);
            firing_lane=fmaxf(firing_lane,alignment);
        }
    }
    reward+=0.04f*firing_lane;
    if(++g->tick>=LIMIT) g->done=true;
    return reward;
}
static int choose(int s,float epsilon) {
    if(random_f()<epsilon) return (int)(random_u()%ACTIONS);
    int best=0,n=1;
    for(int a=1;a<ACTIONS;a++) {
        if(agent.q[s][a]>agent.q[s][best]) { best=a; n=1; }
        else if(agent.q[s][a]==agent.q[s][best] && random_u()%(uint32_t)++n==0) best=a;
    }
    return best;
}
static void update(int s,int a,float r,int next,bool terminal) {
    float best=agent.q[next][0];
    for(int i=1;i<ACTIONS;i++) if(agent.q[next][i]>best) best=agent.q[next][i];
    float target=r+(terminal?0:0.98f*best);
    agent.q[s][a]+=0.12f*(target-agent.q[s][a]);
}
static bool save(void) {
    FILE *f=fopen(MODEL ".tmp","wb");
    if(!f) { perror(MODEL); return false; }
    bool ok=fwrite("SRL2",1,4,f)==4 && fwrite(&agent,sizeof agent,1,f)==1;
    if(fclose(f)!=0) ok=false;
    if(ok && rename(MODEL ".tmp",MODEL)==0) return true;
    fprintf(stderr,"Could not save policy\n"); return false;
}
static bool load(void) {
    FILE *f=fopen(MODEL,"rb"); if(!f) return false;
    char magic[4];
    bool ok=fread(magic,1,4,f)==4 && memcmp(magic,"SRL2",4)==0 && fread(&agent,sizeof agent,1,f)==1;
    fclose(f);
    if(ok) for(int s=0;s<STATES && ok;s++) for(int a=0;a<ACTIONS;a++) if(!isfinite(agent.q[s][a])) { ok=false; break; }
    if(!ok) { memset(&agent,0,sizeof agent); fprintf(stderr,"Invalid model; starting fresh\n"); }
    return ok;
}
static float epsilon(void) { return 0.05f+0.95f*expf(-(float)agent.episodes/2500); }
static float episode(bool learn,bool random_policy,int *kills,int *hits,int *spawned) {
    Game g; reset(&g); float total=0;
    while(!g.done) {
        int s=state(&g),a=random_policy?(int)(random_u()%ACTIONS):choose(s,learn?epsilon():0);
        float r=step(&g,a); if(learn) update(s,a,r,state(&g),g.done); total+=r;
    }
    *kills=g.kills; *hits=g.hits; *spawned=g.spawned;
    if(learn) agent.episodes++;
    return total;
}
static void evaluate(int n,bool baseline) {
    rng=987654321; double total=0; int kills=0,hits=0,spawned=0;
    for(int i=0;i<n;i++) { int k,h,f; total+=episode(false,baseline,&k,&h,&f); kills+=k; hits+=h; spawned+=f; }
    printf("%s: %d episodes, reward %.2f, kills %.2f, hits %.2f (means), kill rate %.1f%%\n",baseline?"Random":"Learned",n,total/n,(double)kills/n,(double)hits/n,spawned?100.0*kills/spawned:0.0);
}
static void tests(void) {
    Game left,right; reset(&left); reset(&right);
    left.x=50; right.x=W-50; left.spawn=right.spawn=100;
    left.cooldown=right.cooldown=100;
    left.enemies[0]=right.enemies[0]=(Body){.x=399,.y=100,.active=true,
        .hp=50,.vx=200,.vy=10,.patrol_left=50,.patrol_right=400};
    step(&left,4); step(&right,4);
    assert(left.enemies[0].x==right.enemies[0].x);
    assert(left.enemies[0].vx<0 && left.enemies[0].x<=400);
    Game spawned; reset(&spawned); step(&spawned,4);
    assert(spawned.spawned==1);
    step(&spawned,4); assert(spawned.spawned==1);
    Game g; reset(&g); assert(state(&g)>=0 && state(&g)<STATES);
    g.spawn=100; g.cooldown=100;
    g.enemies[0]=(Body){.x=g.x,.y=g.y,.active=true,.hp=25};
    step(&g,4); assert(g.health==START_HEALTH-5 && g.hits==1);
    reset(&g); g.spawn=100; g.cooldown=100;
    g.enemies[0]=(Body){.x=g.x,.y=g.y-100,.active=true,.hp=1}; g.bullets[0]=(Body){.x=g.x,.y=g.y-85,.active=true};
    step(&g,4); assert(g.kills==1 && !g.enemies[0].active && g.landed==1);
    assert(g.blasts[0].active);
    reset(&g); g.spawn=100; g.cooldown=100;
    g.blasts[0]=(Body){.x=g.x,.y=g.y,.active=true,.ttl=36};
    step(&g,4); assert(g.hits==1 && g.health==START_HEALTH-1);
    step(&g,4); assert(g.hits==1); /* explosion cooldown prevents per-frame damage */
    reset(&g); g.spawn=100; g.blasts[0]=(Body){.y=H+1,.active=true};
    step(&g,4); assert(!g.blasts[0].active);
    update(0,0,2,1,true);
 assert(fabsf(agent.q[0][0]-0.24f)<0.0001f);
    memset(&agent,0,sizeof agent);
    for(int i=0;i<100;i++) {
        reset(&g);
        while(!g.done) { int s=state(&g); assert(s>=0 && s<STATES); step(&g,(int)(random_u()%ACTIONS)); }
        assert(g.tick<=LIMIT && g.health>=0);
    }
    puts("PASS: damage, bullet kills, explosion damage/cooldown/expiry, terminal Q target, 100 simulations");
}
static Texture2D sprite(const char *name) {
    Texture2D t=LoadTexture(TextFormat("assets/%s.png",name));
    if(t.id) SetTextureFilter(t,TEXTURE_FILTER_BILINEAR);
    return t;
}
static void draw_sprite(Texture2D t,float x,float y,float width,float height,Color tint) {
    if(!t.id) { DrawCircle((int)x,(int)y,width/2,tint); return; }
    DrawTexturePro(t,(Rectangle){0,0,(float)t.width,(float)t.height},
        (Rectangle){x,y,width,height},(Vector2){width/2,height/2},0,tint);
}
static float blend(float previous,float current,float alpha) {
    return previous+(current-previous)*alpha;
}
static void draw_body(Texture2D texture,const Body *previous,const Body *current,
                      float alpha,float width,float height) {
    float dx=current->x-previous->x,dy=current->y-previous->y;
    /* New/reused slots appear at their spawn rather than streaking across the arena. */
    if(!previous->active || dx*dx+dy*dy>100*100) alpha=1;
    draw_sprite(texture,blend(previous->x,current->x,alpha),
                blend(previous->y,current->y,alpha),width,height,WHITE);
}
static void window(void) {

    InitWindow(W,H,"Spaceship RL - independent C project"); SetTargetFPS(120);
    Texture2D ship=sprite("spaceship"),ammo=sprite("ammo1"),blast=sprite("explosion");
    Texture2D enemy_tex[3]={sprite("aliencraft1"),sprite("aliencraft2"),sprite("aliencraft3")};
    Game g; reset(&g); bool learning=false,manual=false,paused=false;
    Game previous=g;
    int speed=1; double accumulator=0;
    while(!WindowShouldClose()) {
        if(IsKeyPressed(KEY_T)) learning=!learning;
        if(IsKeyPressed(KEY_M)) manual=!manual;
        if(IsKeyPressed(KEY_SPACE)) paused=!paused;
        if(IsKeyPressed(KEY_F)) speed=speed==1?10:1;
        if(IsKeyPressed(KEY_R)) { reset(&g); previous=g; accumulator=0; }
        if(IsKeyPressed(KEY_S)) save();
        accumulator+=paused?0:fmin(GetFrameTime(),0.1)*speed;
        while(accumulator>=1.0/30) {
            accumulator-=1.0/30;
            if(g.done) { if(learning && !manual) agent.episodes++; reset(&g); }
            previous=g;
            int s=state(&g),a=choose(s,learning?epsilon():0);
            if(manual) {
                int x=IsKeyDown(KEY_RIGHT)-IsKeyDown(KEY_LEFT),y=IsKeyDown(KEY_DOWN)-IsKeyDown(KEY_UP);
                a=(y+1)*3+x+1;
            }
            float r=step(&g,a);
            if(learning && !manual) update(s,a,r,state(&g),g.done);
        }
        float alpha=(float)(accumulator*30);
        BeginDrawing(); ClearBackground(BLACK);
        for(int i=0;i<ENEMIES;i++) if(g.enemies[i].active) {
            draw_body(enemy_tex[g.enemies[i].type],&previous.enemies[i],&g.enemies[i],alpha,50,50);
        }
        for(int i=0;i<BULLETS;i++) if(g.bullets[i].active)
            draw_body(ammo,&previous.bullets[i],&g.bullets[i],alpha,20,30);
        for(int i=0;i<BLASTS;i++) if(g.blasts[i].active) {
            draw_body(blast,&previous.blasts[i],&g.blasts[i],alpha,100,100);
        }
        draw_sprite(ship,blend(previous.x,g.x,alpha),blend(previous.y,g.y,alpha),50,50,WHITE);

        DrawRectangle(0,0,W,54,(Color){15,24,44,255});
        DrawText(TextFormat("%s  HP %d   Kills %d   Score %d   Kill rate %.1f%%   Ep %u   %dx",manual?"MANUAL":learning?"TRAINING":"POLICY",g.health,g.kills,g.score,g.spawned?100.0*g.kills/g.spawned:0.0,agent.episodes,speed),16,8,20,RAYWHITE);
        DrawText("T train | M manual | arrows move | F speed | Space pause | R reset | S save",16,34,14,GRAY);
        EndDrawing();
    }
    save(); UnloadTexture(ship); UnloadTexture(ammo); UnloadTexture(blast);
    for(int i=0;i<3;i++) UnloadTexture(enemy_tex[i]);
    CloseWindow();
}
int main(int argc,char **argv) {
    if(argc==2 && strcmp(argv[1],"--test")==0) { tests(); return 0; }
    load();
    if(argc==3 && (strcmp(argv[1],"--train")==0 || strcmp(argv[1],"--eval")==0)) {
        char *end; long n=strtol(argv[2],&end,10);
        if(*end || n<1 || n>1000000) { fprintf(stderr,"Episode count must be 1..1000000\n"); return 1; }
        if(strcmp(argv[1],"--eval")==0) { evaluate((int)n,true); evaluate((int)n,false); return 0; }
        double total=0;
        for(long i=0;i<n;i++) {
            int k,h,f; total+=episode(true,false,&k,&h,&f);
            if((i+1)%1000==0) { printf("Episodes %u | epsilon %.3f | mean reward %.2f\n",agent.episodes,epsilon(),total/1000); total=0; }
        }
        return save()?0:1;
    }
    if(argc!=1) { fprintf(stderr,"Usage: %s [--train N | --eval N | --test]\nRun from AI_Project.\n",argv[0]); return 1; }
    window(); return 0;
}
