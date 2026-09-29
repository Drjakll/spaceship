#include "spaceship_view.h"
#include <stdio.h>
#include <string.h>
static const Color BACK={8,13,24,255}, PANEL={16,25,41,255}, INK={229,238,246,255}, MUTED={132,153,177,255};
static const Color COLORS[8]={{83,218,222,255},{251,189,103,255},{192,154,255,255},{132,218,151,255},
    {255,137,157,255},{132,175,255,255},{236,212,139,255},{188,215,217,255}};
static void text(const SpaceView *v,const char *message,float x,float y,float size,Color color) {
    DrawTextEx(v->font,message,(Vector2){x,y},size,.4f,color);
}
static void sprite(Texture2D texture,float x,float y,float width,float height,Color tint) {
    DrawTexturePro(texture,(Rectangle){0,0,(float)texture.width,(float)texture.height},
        (Rectangle){24+x*.75f-width/2,64+y*.75f-height/2,width,height},(Vector2){0,0},0,tint);
}
bool space_view_open(SpaceView *view) {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1160,846,"Spaceship | Cooperative MARL");
    if(!IsWindowReady()) return false;
    SetTargetFPS(60);
    view->ship=LoadTexture("Pictures/spaceship.png");
    view->enemies[0]=LoadTexture("Pictures/aliencraft1.png");
    view->enemies[1]=LoadTexture("Pictures/aliencraft2.png");
    view->enemies[2]=LoadTexture("Pictures/aliencraft3.png");
    view->missile=LoadTexture("Pictures/ammo1.png");
    view->explosion=LoadTexture("Pictures/explosion.png");
    view->font=LoadFontEx("Fonts/NotoSans-VariableFont.ttf",48,NULL,0);
    if(!view->ship.id || !view->missile.id || !view->explosion.id || !view->font.texture.id) return false;
    for(int i=0;i<3;++i) if(!view->enemies[i].id) return false;
    SetTextureFilter(view->ship,TEXTURE_FILTER_BILINEAR);
    for(int i=0;i<3;++i) SetTextureFilter(view->enemies[i],TEXTURE_FILTER_BILINEAR);
    return true;
}
void space_view_draw(const SpaceView *v,const SpaceWorld *w,const char *policy,bool paused) {
    BeginDrawing();ClearBackground(BACK);
    text(v,"SPACESHIP",24,16,25,INK);
    text(v,"COOPERATIVE DEFENSE",191,22,13,MUTED);
    text(v,paused?"PAUSED":"LIVE",710,21,13,paused?COLORS[1]:COLORS[0]);
    DrawRectangle(24,64,750,750,(Color){10,18,32,255});
    BeginScissorMode(24,64,750,750);
    for(int i=0;i<110;++i) DrawCircle(30+(i*127)%737,70+(i*211)%737,i%7==0?1.4f:.65f,(Color){79,105,132,160});
    for(int a=1;a<w->config.num_agents;++a) DrawLine(24+750*a/w->config.num_agents,64,
        24+750*a/w->config.num_agents,814,(Color){26,40,59,255});
    for(int i=0;i<SPACE_MAX_ENEMIES;++i) {
        const SpaceEnemy *e=&w->enemies[i];
        if(e->phase==1) {
            sprite(v->enemies[e->type],e->x,e->y,54,54,WHITE);
            DrawRectangle(24+e->x*.75f-15,64+e->y*.75f+23,30,3,(Color){63,46,52,255});
            const float max_hp[]={10,15,25};
            DrawRectangle(24+e->x*.75f-15,64+e->y*.75f+23,30*e->health/max_hp[e->type],3,COLORS[4]);
        } else if(e->phase==2) sprite(v->explosion,e->x,e->y,90,90,Fade(WHITE,.65f));
    }
    for(int i=0;i<SPACE_MAX_PROJECTILES;++i) if(w->projectiles[i].active) {
        const SpaceProjectile *p=&w->projectiles[i];sprite(v->missile,p->x,p->y,13,25,COLORS[p->owner]);
    }
    for(int a=0;a<w->config.num_agents;++a) {
        const SpaceShip *s=&w->ships[a];
        if(s->health<=0) continue;
        DrawCircleLines(24+s->x*.75f,64+s->y*.75f,27,Fade(COLORS[a],.5f));
        sprite(v->ship,s->x,s->y,55,55,WHITE);
        char id[8];snprintf(id,sizeof(id),"%d",a+1);text(v,id,20+s->x*.75f,87+s->y*.75f,13,COLORS[a]);
    }
    EndScissorMode();
    DrawRectangleLines(24,64,750,750,(Color){40,58,78,255});
    DrawRectangle(798,64,338,750,PANEL);
    text(v,"MISSION CONTROL",818,84,20,INK);
    text(v,policy,818,116,12,COLORS[0]);
    char line[160];
    snprintf(line,sizeof(line),"%d ALLIES  /  SCRIPTED ENEMIES",w->config.num_agents);text(v,line,818,141,12,MUTED);
    DrawLine(818,174,1116,174,(Color){40,58,78,255});
    snprintf(line,sizeof(line),"%03d",w->killed);text(v,line,818,191,44,INK);text(v,"DESTROYED",818,243,11,MUTED);
    snprintf(line,sizeof(line),"%03d",w->escaped);text(v,line,997,191,44,w->escaped?COLORS[4]:INK);text(v,"ESCAPED",997,243,11,MUTED);
    snprintf(line,sizeof(line),"Wave  %d / %d",w->spawned,w->config.wave_size);text(v,line,818,280,16,INK);
    DrawRectangle(818,308,298,5,(Color){39,53,72,255});
    DrawRectangle(818,308,298*w->spawned/w->config.wave_size,5,COLORS[0]);
    snprintf(line,sizeof(line),"Time  %.1fs     Score  %d",w->tick/120.0,w->score);text(v,line,818,332,14,MUTED);
    snprintf(line,sizeof(line),"Team return  %+.2f",w->episode_return);text(v,line,818,357,14,INK);
    snprintf(line,sizeof(line),"Escapes  %.1f%%  /  target <= 10%%",100.0*w->escaped/w->config.wave_size);text(v,line,818,382,13,MUTED);
    int spacing=w->config.num_agents<=4?59:34;
    for(int a=0;a<w->config.num_agents;++a) {
        const SpaceShip *s=&w->ships[a];int y=428+a*spacing;
        DrawCircle(824,y+8,4,COLORS[a]);
        snprintf(line,sizeof(line),"SHIP %d     HP %.0f     K %d  A %d",a+1,s->health,s->kills,s->assists);
        text(v,line,838,y,12,s->health>0?INK:MUTED);
        DrawRectangle(838,y+22,270,3,(Color){39,53,72,255});
        DrawRectangle(838,y+22,270*s->health/50,3,COLORS[a]);
    }
    if(w->terminal) {
        snprintf(line,sizeof(line),"COMPLETE  /  failed defense %.1f%%",100*space_failure_fraction(w));
        text(v,line,818,724,13,COLORS[1]);
    }
    text(v,"P  pause     R  new seed     ESC  exit",818,772,12,MUTED);
    text(v,"30 decisions/s  /  120 physics ticks/s",24,823,11,MUTED);
    EndDrawing();
}
void space_view_close(SpaceView *view) {
    UnloadTexture(view->ship);UnloadTexture(view->missile);UnloadTexture(view->explosion);
    for(int i=0;i<3;++i) UnloadTexture(view->enemies[i]);
    UnloadFont(view->font);CloseWindow();
}
