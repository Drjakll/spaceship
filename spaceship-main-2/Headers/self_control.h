#ifndef SPACESHIP_SELF_CONTROL_H
#define SPACESHIP_SELF_CONTROL_H

#include "types.h"

struct Spaceship_Control_Probability {
  float horizontal[3];
  float vertical[3];
  float shoot[2];
};

#endif
