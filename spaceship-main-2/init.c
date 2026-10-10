#include "Headers/game.h"

/**
 * @brief Delegate program startup and shutdown to the game lifecycle.
 * @return The exit status returned by game_run().
 */
int main(void) {
  return game_run();
}
