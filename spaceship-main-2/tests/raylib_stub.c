#include <raylib.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Deterministic graphics/input boundary for original/refactor comparisons. */
static uint64_t trace_hash = UINT64_C(14695981039346656037);
static uint32_t random_state = 42;
static unsigned int texture_id;
static int frame;
static int clock_tick;

static void record(const void *bytes, size_t length) {
  const unsigned char *data = bytes;
  for (size_t i = 0; i < length; i++) {
    trace_hash ^= data[i];
    trace_hash *= UINT64_C(1099511628211);
  }
}

static void record_text(const char *text) {
  record(text, strlen(text) + 1);
}

void InitWindow(int width, int height, const char *title) {
  record(&width, sizeof(width));
  record(&height, sizeof(height));
  record_text(title);
}

bool WindowShouldClose(void) {
  return frame >= 3600;
}

void SetTargetFPS(int fps) {
  record(&fps, sizeof(fps));
}

double GetTime(void) {
  return clock_tick++ / 120.0;
}

int GetRandomValue(int minimum, int maximum) {
  random_state = random_state * 1664525u + 1013904223u;
  int result = minimum + (int)(random_state % (maximum - minimum + 1));
  record(&result, sizeof(result));
  return result;
}

Image LoadImage(const char *path) {
  record_text(path);
  return (Image){.width = 64, .height = 64, .mipmaps = 1};
}

void ImageResize(Image *image, int width, int height) {
  image->width = width;
  image->height = height;
  record(&width, sizeof(width));
  record(&height, sizeof(height));
}

Texture2D LoadTextureFromImage(Image image) {
  return (Texture2D){
    .id = ++texture_id,
    .width = image.width,
    .height = image.height,
    .mipmaps = 1
  };
}

void UnloadImage(Image image) {
  record(&image.width, sizeof(image.width));
  record(&image.height, sizeof(image.height));
}

void UnloadTexture(Texture2D texture) {
  record(&texture.id, sizeof(texture.id));
}

Font LoadFont(const char *path) {
  record_text(path);
  return (Font){.baseSize = 24};
}

void UnloadFont(Font font) {
  record(&font.baseSize, sizeof(font.baseSize));
}

void BeginDrawing(void) {
  record_text("begin");
}

void ClearBackground(Color color) {
  record(&color, sizeof(color));
}

void DrawTexture(Texture2D texture, int x, int y, Color tint) {
  record(&texture.id, sizeof(texture.id));
  record(&x, sizeof(x));
  record(&y, sizeof(y));
  record(&tint, sizeof(tint));
}

void DrawTextEx(Font font, const char *text, Vector2 position,
                float size, float spacing, Color tint) {
  record(&font.baseSize, sizeof(font.baseSize));
  record_text(text);
  record(&position.x, sizeof(position.x));
  record(&position.y, sizeof(position.y));
  record(&size, sizeof(size));
  record(&spacing, sizeof(spacing));
  record(&tint, sizeof(tint));
}

void EndDrawing(void) {
  printf("frame %d: %016llx\n", frame++, (unsigned long long)trace_hash);
}

void CloseWindow(void) {
  printf("closed: %016llx\n", (unsigned long long)trace_hash);
}
