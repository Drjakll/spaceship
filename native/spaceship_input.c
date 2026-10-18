#include "spaceship_input.h"
SpaceAction space_manual_action(bool up,bool down,bool left,bool right,bool fire) {
    static const int moves[3][3]={{SPACE_UP_LEFT,SPACE_UP,SPACE_UP_RIGHT},
        {SPACE_LEFT,SPACE_STAY,SPACE_RIGHT},{SPACE_DOWN_LEFT,SPACE_DOWN,SPACE_DOWN_RIGHT}};
    SpaceAction action={moves[(int)down-(int)up+1][(int)right-(int)left+1],fire?1:0};
    return action;
}
