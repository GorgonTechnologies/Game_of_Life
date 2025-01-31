#define GL_GLEXT_PROTOTYPES
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>
#include <cglm/cglm.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef UTILS_H
#define UTILS_H

typedef struct{
  unsigned char color[3];
  float x; float y;
  float flip;
  float state;
} Cell;

typedef enum {
  TRIGON,
  TETRAGON,
  HEXAGON
} Mode;

typedef enum {
  L,
  M,
  S,
  XS
} Size;

typedef struct{
  unsigned char data[12];
  char size;
} u8_array;

typedef struct{
  float data[12];
  char size;
} f32_array;

typedef struct{
  Cell* data;
  int rows;
  int cols;
  Mode mode;
  int generation;
} Grid;

GLuint create_grid_texture(
  int line_width,
  int spacing);

GLuint compile_shader(
  GLenum type,
  const char* source);

GLuint create_shader_program(
  const char* vertexSource,
  const char* fragmentSource);

static inline void APIENTRY GL_debug_callback(
  GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  GLsizei length,
  const GLchar* message,
  const void* userParam){
    fprintf(stderr, "OpenGL Debug: %s\n", message);
}

static inline void init_f32_array(
  f32_array* dst,
  float* src,
  char size){
    dst->size = size;
    memcpy(dst->data, src, sizeof(float) * size);
}

static inline void init_u8_array(
  u8_array* dst,
  unsigned char* src,
  char size){
    dst->size = size;
    memcpy(dst->data, src, sizeof(unsigned char) * size);
}

void hash(
  unsigned int row,
  unsigned int col,
  unsigned char out[3]);

void rehash(
  unsigned char color[3],
  unsigned int out[2]);

int sdl_init(
    char* name,
    SDL_Window** window,
    SDL_GLContext* context,
    int width,
    int height);

void ui_init(
  GLuint program,
  int width,
  int height);

void ui_left_click(
  int x,
  int y,
  Mode* mode,
  int* play);

void ui_right_click(
  int x,
  int y,
  Mode mode,
  Size* size);

void ui_left_render(Mode mode, int play);

void ui_right_render(Mode mode, Size size);

void ui_destroy();

void game_init(
  GLuint program,
  Mode mode,
  Size size,
  int width,
  int height);

void game_render(
  Mode mode);

void game_click(
  int x,
  int y,
  Mode* mode);

void game_destroy();

void next_generation(char u, char o, char r);

#endif
