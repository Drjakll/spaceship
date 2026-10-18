#include "assets.h"

static const char *const enemy_image_paths[ENEMY_VARIANT_COUNT] = {
  "Pictures/aliencraft1.png",
  "Pictures/aliencraft2.png",
  "Pictures/aliencraft3.png"
};

/** @copydoc assets_load */
void assets_load(Game_Assets *assets) {
  /* Keep image loads before texture uploads, matching original startup. */
  for (int i = 0; i < ENEMY_VARIANT_COUNT; i++) {
    assets->enemy_images[i] = LoadImage(enemy_image_paths[i]);
    ImageResize(&assets->enemy_images[i], ENEMY_RADIUS * 2, ENEMY_RADIUS * 2);
  }
  for (int i = 0; i < ENEMY_VARIANT_COUNT; i++) {
    assets->enemy_models[i] = LoadTextureFromImage(assets->enemy_images[i]);
  }

  assets->ammo_image = LoadImage("Pictures/ammo1.png");
  ImageResize(&assets->ammo_image, MISSILE_RADIUS * 2, MISSILE_RADIUS * 3);
  assets->ammo_model = LoadTextureFromImage(assets->ammo_image);

  assets->explosion_image = LoadImage("Pictures/explosion.png");
  ImageResize(&assets->explosion_image,
              EXPLOSION_RADIUS * 2, EXPLOSION_RADIUS * 2);
  assets->explosion_model = LoadTextureFromImage(assets->explosion_image);
}

/** @copydoc assets_load_spaceship_model */
Texture assets_load_spaceship_model(void) {
  Image image = LoadImage("Pictures/spaceship.png");
  ImageResize(&image, SPACESHIP_RADIUS * 2, SPACESHIP_RADIUS * 2);
  Texture model = LoadTextureFromImage(image);
  UnloadImage(image);
  return model;
}

/** @copydoc assets_unload */
void assets_unload(Game_Assets *assets) {
  for (int i = 0; i < ENEMY_VARIANT_COUNT; i++) {
    UnloadImage(assets->enemy_images[i]);
  }
  UnloadImage(assets->ammo_image);
  UnloadImage(assets->explosion_image);

  for (int i = 0; i < ENEMY_VARIANT_COUNT; i++) {
    UnloadTexture(assets->enemy_models[i]);
  }
  UnloadTexture(assets->ammo_model);
  UnloadTexture(assets->explosion_model);
}
