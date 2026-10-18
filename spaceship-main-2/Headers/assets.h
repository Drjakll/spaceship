#ifndef SPACESHIP_ASSETS_H
#define SPACESHIP_ASSETS_H

#include <raylib.h>
#include "macros.h"

typedef struct Game_Assets {
  Image enemy_images[ENEMY_VARIANT_COUNT];
  Texture enemy_models[ENEMY_VARIANT_COUNT];
  Image ammo_image;
  Image explosion_image;
  Texture ammo_model;
  Texture explosion_model;
} Game_Assets;

/**
 * @brief Load and resize shared enemy, missile, and explosion assets.
 * @param assets Destination that owns the resulting images and textures.
 * @pre A raylib window is open; the project root is the working directory.
 * @note Release these resources with assets_unload() before closing the window.
 */
void assets_load(Game_Assets *assets);

/**
 * @brief Release the shared images and textures loaded by assets_load().
 * @param assets Loaded asset collection whose resources will be released.
 * @pre Resources have not already been unloaded; the raylib window is open.
 * @note Spaceship textures and the HUD font are released by game_shutdown().
 */
void assets_unload(Game_Assets *assets);

/**
 * @brief Load a spaceship texture at the configured spaceship dimensions.
 * @return A texture the caller must release with UnloadTexture().
 * @pre A raylib window is open; the project root is the working directory.
 * @note The intermediate CPU image is released before this function returns.
 */
Texture assets_load_spaceship_model(void);

#endif
