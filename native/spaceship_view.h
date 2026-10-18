#ifndef SPACESHIP_VIEW_H
#define SPACESHIP_VIEW_H
#include "raylib.h"
#include "spaceship_core.h"
typedef struct {Texture2D ship, enemies[3], missile, explosion; Font font;} SpaceView;
bool space_view_open(SpaceView *view);
void space_view_draw(const SpaceView *view, const SpaceWorld *world, const char *policy, bool paused);
void space_view_close(SpaceView *view);
#endif
