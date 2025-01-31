#include "utils.h"

// OpenGL

#define texture_width 64
#define texture_height 64
unsigned char texture_data
  [texture_width]
  [texture_height]
  [4];

GLuint create_grid_texture(int line_width, int spacing) {
  for (unsigned char x = 0; x < texture_width; x++) 
    for (unsigned char y = 0; y < texture_height; y++) {
      bool isLine = (x % (line_width + spacing) < line_width)
        || (y % (line_width + spacing) < line_width);
      
      texture_data[x][y][0] = (GLubyte) isLine ? 255 : 0;
      texture_data[x][y][1] = (GLubyte) isLine ? 255 : 0;
      texture_data[x][y][2] = (GLubyte) isLine ? 255 : 0;
      texture_data[x][y][3] = (GLubyte) isLine ? 255 : 0;
    }

  GLuint texture;
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width, texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glBindTexture(GL_TEXTURE_2D, 0);
  return texture;
}

GLuint compile_shader(GLenum type, const char* source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  int success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    fprintf(stderr, "Error: Shader compilation failed\n%s\n", infoLog);
    return 0;
  }
  return shader;
}

GLuint create_shader_program(const char* vertex_shader_source, const char* fragment_shader_source) {
  GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
  GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);

  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  int success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(program, 512, NULL, infoLog);
    fprintf(stderr, "Error: Program linking failed\n%s\n", infoLog);
    return 0;
  }

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  return program;
}

// Hashing
void to_binary(unsigned int value, char len, char* out) {
  // {js} Number(value).toString(2);
  for (int i = len-1; i >= 0; --i) {
    out[i] = (value & 1) ? '1' : '0';
    value >>= 1;
  }
}

unsigned int from_binary(char* value, char len) {
  // {js} parseInt(value, 2);
  unsigned int result = 0;
  for (int i = 0; i < len; i++) {
    result <<= 1; 
    if (value[i] == '1')
      result |= 1;
  }
  return result;
}

void hash(unsigned int row, unsigned int col, unsigned char out[3]){
  // row           col                     color
  // 4095          4095                =>  255,      255,      255
  // 111111111111  111111111111            11111111, 11111111, 11111111 
  // 11111111 | 1111  1111 | 11111111
  //   -> R        -> G        -> B
  unsigned char step = 20;
  char rs[12] = {}; to_binary(row * step, 12, rs); 
  char cs[12] = {}; to_binary(col * step, 12, cs);

  char r[8] = {};
  char g[8] = {};
  char b[8] = {};

  for(unsigned char i=0; i<8; i++)            r[i] = rs[i];
  for(unsigned char i=8, j=0; i<12; i++, j++) g[j] = rs[i];
  for(unsigned char i=0, j=4; i<4; i++, j++)  g[j] = cs[i];
  for(unsigned char i=4, j=0; i<12; i++, j++) b[j] = cs[i];

  out[0] = from_binary(r, 8);
  out[1] = from_binary(g, 8);
  out[2] = from_binary(b, 8);
}

void rehash(unsigned char color[3], unsigned int out[2]){
  // color                                row           col
  // 255,      255,      255          =>  4095          4095
  // 11111111, 11111111, 11111111         111111111111  111111111111
  // 11111111  1111 | 1111  11111111
  //     -> ROW         -> COL 
  unsigned char step = 20;

  char r[8] = {}; to_binary(color[0], 8, r);
  char g[8] = {}; to_binary(color[1], 8, g);
  char b[8] = {}; to_binary(color[2], 8, b);

  char rs[12] = {};
  char cs[12] = {};

  for(unsigned char i=0; i<8; i++)            rs[i] = r[i];
  for(unsigned char i=8, j=0; i<12; i++, j++) rs[i] = g[j];
  for(unsigned char i=0, j=4; i<4; i++, j++)  cs[i] = g[j];
  for(unsigned char i=4, j=0; i<12; i++, j++) cs[i] = b[j];
 
  out[0] = from_binary(rs, 12) / step;
  out[1] = from_binary(cs, 12) / step;
}

// SDL
int sdl_init(char* name, SDL_Window** window, SDL_GLContext* context, int width, int height){
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return -1;
  }

  *window = SDL_CreateWindow(name, width, height, SDL_WINDOW_OPENGL);
  if (!*window) {
    fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
    SDL_Quit();
    return -1;
  }

  *context = SDL_GL_CreateContext(*window);
  if (!*context) {
    fprintf(stderr, "SDL_GL_CreateContext Error: %s\n", SDL_GetError());
    SDL_DestroyWindow(*window);
    SDL_Quit();
    return -1;
  }
  return 1;
}

