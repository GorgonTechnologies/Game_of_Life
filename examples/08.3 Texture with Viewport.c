#define GL_GLEXT_PROTOTYPES
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>
#include <cglm/cglm.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#ifndef _TYPES_
#define _TYPES_
  #include <stdint.h>
  #include <stdbool.h>
  // Aliases
  typedef int64_t i64; 
  typedef int32_t i32;
  typedef int16_t i16;
  typedef int8_t i8;
  typedef uint64_t u64; 
  typedef uint32_t u32;
  typedef uint16_t u16;
  typedef uint8_t u8;
  typedef float f32;
  typedef double f64;
  // typedef bool bool;
#endif

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define size 60.0f
f32 vertices[16] = {
  //Vertices    //Texture
 -size, size,   0.0f, 0.00f,
  size, size,   1.0f, 0.00f,
  size,-size,   1.0f, 1.00f,
 -size,-size,   0.0f, 1.00f,
};

u8 indices_w[8] = {
  0, 1, 1, 2, 2, 3, 3, 0
};

u8 indices_s[6] = {
  0, 1, 2, 0, 2, 3
};

f32 positions[12] = {
  SCREEN_WIDTH * 0.25,  SCREEN_HEIGHT - SCREEN_HEIGHT * 0.25,  
  SCREEN_WIDTH * 0.50,  SCREEN_HEIGHT - SCREEN_HEIGHT * 0.25,   
  SCREEN_WIDTH * 0.75,  SCREEN_HEIGHT - SCREEN_HEIGHT * 0.25,  
  SCREEN_WIDTH * 0.25,  SCREEN_HEIGHT - SCREEN_HEIGHT * 0.75,  
  SCREEN_WIDTH * 0.50,  SCREEN_HEIGHT - SCREEN_HEIGHT * 0.75,   
  SCREEN_WIDTH * 0.75,  SCREEN_HEIGHT - SCREEN_HEIGHT * 0.75,    
};

f32 states[6] = {
  1.0, 0.0, 0.0,
  0.0, 1.0, 0.0, 
};

mat4 projection;

const char* vertexShaderSource = 
  "#version 330 core                           \n"

  "layout(location = 0) in vec2 aPos;          \n"
  "layout(location = 1) in vec2 aTex;          \n"
  "layout(location = 2) in vec2 iPos;          \n"
  "layout(location = 3) in float iState;       \n"

  "uniform float uState;                       \n"
  "uniform mat4 uProjection;                   \n"

  "out float fragState;                        \n"
  "out vec2 texCoord;                          \n"

  "void main() {                               \n"
  "  float dx = 0;                             \n"

  "  if((iState + uState) < 1.0)               \n"
  "      dx = -10000.0;                        \n"
  
  "  gl_Position = uProjection *               \n"
  "    vec4(                                   \n"
  "     aPos.x + iPos.x + dx,                  \n"
  "     aPos.y + iPos.y,                       \n"
  "     0.0,                                   \n"
  "     1.0);                                  \n"

  "  fragState = uState > 0 ? 0.0 : iState;    \n"
  "  texCoord = vec2(aTex.x, aTex.y);          \n"
  "}";

const char* fragmentShaderSource =
  "#version 330 core                           \n"
  "in float fragState;                         \n"
  "in vec2 texCoord;                           \n"
  
  "out vec4 fragColor;                         \n"
  
  "uniform sampler2D uTexture;                 \n"

  "void main() {                               \n"
  "  if(fragState < 1.0)                       \n"
  "    fragColor = vec4(1.0, 1.0, 1.0, 1.0);   \n"
  "  else                                      \n"
  "    fragColor = texture(                    \n"
  "      uTexture, texCoord);                  \n"
  "}";

void checkShaderCompilation(GLuint shader) {
  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
    fprintf(stderr, "Shader compilation error: %s\n", infoLog);
    exit(EXIT_FAILURE);
  }
}

void checkProgramLinking(GLuint program) {
  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
    fprintf(stderr, "Program linking error: %s\n", infoLog);
    exit(EXIT_FAILURE);
  }
}

int main() {
  SDL_Window* window;
  SDL_GLContext context;
  { 
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
      return -1;
    }

    window = SDL_CreateWindow(
      "SDL3 OpenGL Demo",
      SCREEN_WIDTH,
      SCREEN_HEIGHT,
      SDL_WINDOW_OPENGL);
    context = SDL_GL_CreateContext(window);
    if (!context) {
      fprintf(stderr, "Error creating OpenGL context: %s\n", SDL_GetError());
      SDL_Quit();
      return -1;
    }

    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1);
  }

  GLuint VAO, VBO, EBO, stateVBO, positionVBO;
  { 
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &stateVBO);
    glGenBuffers(1, &positionVBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
      GL_ARRAY_BUFFER,
      sizeof(vertices),
      vertices,
      GL_STATIC_DRAW);
    glVertexAttribPointer(
      0,
      2,
      GL_FLOAT,
      GL_FALSE,
      4 * sizeof(float),
      (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
      1,               // Attribute index (matches layout(location = 1))
      2,               // 2 components (s, t)
      GL_FLOAT,        // Data type
      GL_FALSE,        // Normalize?
      4 * sizeof(float), // Stride (skip x, y)
      (void*)(2 * sizeof(float)) // Offset (start at s, t)
    );
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
    glBufferData(
      GL_ARRAY_BUFFER,
      sizeof(positions),
      positions,
      GL_STATIC_DRAW);
    glVertexAttribPointer(
      2,
      2,
      GL_FLOAT,
      GL_FALSE,
      2 * sizeof(float),
      (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glBindBuffer(GL_ARRAY_BUFFER, stateVBO);
    glBufferData(
      GL_ARRAY_BUFFER,
      sizeof(states),
      states,
      GL_STATIC_DRAW);
    glVertexAttribPointer(
      3,
      1,
      GL_FLOAT,
      GL_FALSE,
      1 * sizeof(float),
      (void*)0);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);
  }

  GLuint shaderProgram;
  { 
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    checkShaderCompilation(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    checkShaderCompilation(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkProgramLinking(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
  }
  
  GLuint uStateLoc, uTextureLoc, uProjLoc;
  {
    glm_ortho(0, SCREEN_WIDTH, 0, SCREEN_HEIGHT, -1.0f, 1.0f, projection);
    uStateLoc = glGetUniformLocation(shaderProgram, "uState");
    uTextureLoc = glGetUniformLocation(shaderProgram, "uTexture");
    uProjLoc = glGetUniformLocation(shaderProgram, "uProjection");
  }

  #define texture_width 64
  #define texture_height 64
  unsigned char texture_data
    [texture_width]
    [texture_height]
    [4];

  GLuint texture;
  {
    u8 line_width = 1;
    u8 spacing = 8;
    
    for (unsigned char x = 0; x < texture_width; x++) 
      for (unsigned char y = 0; y < texture_height; y++) {
        bool isLine = (x % (line_width + spacing) < line_width)
                || (y % (line_width + spacing) < line_width);
      
        texture_data[x][y][0] = (GLubyte) isLine ? 255 : 0;
        texture_data[x][y][1] = (GLubyte) isLine ? 255 : 0;
        texture_data[x][y][2] = (GLubyte) isLine ? 255 : 0;
        texture_data[x][y][3] = (GLubyte) isLine ? 255 : 0;
      }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA,
      texture_width,
      texture_height,
      0,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      texture_data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE0);

    printf("Texture ID: %d\n", texture);

    GLint texWidth, texHeight;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);
    printf("Texture Size: %d x %d\n", texWidth, texHeight);

    GLint activeTex;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTex);
    printf("Active Texture: %d\n", activeTex - GL_TEXTURE0);

    GLint boundTex;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);
    printf("Bound Texture: %d\n", boundTex);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
      printf("OpenGL Error: %d\n", err);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
  }

  int running = 1;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = 0;
      }
    }
      
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    glUseProgram(shaderProgram);

    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(uTextureLoc, 0);
    glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, (const GLfloat*)projection);

    glBindVertexArray(VAO);
      
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
    
    glUniform1f(uStateLoc, 0.0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
      GL_ELEMENT_ARRAY_BUFFER,
      sizeof(indices_s),
      indices_s,
      GL_STATIC_DRAW);
    glDrawElementsInstanced(
      GL_TRIANGLES,
      21,
      GL_UNSIGNED_BYTE,
      0,
      6);

    glDisable(GL_BLEND);

    glUniform1f(uStateLoc, 1.0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
      GL_ELEMENT_ARRAY_BUFFER,
      sizeof(indices_w),
      indices_w,
      GL_STATIC_DRAW);
    glDrawElementsInstanced(
      GL_LINES,
      26,
      GL_UNSIGNED_BYTE,
      0,
      6);

    SDL_GL_SwapWindow(window);
  }

  {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &stateVBO);
    glDeleteBuffers(1, &positionVBO);
    glDeleteProgram(shaderProgram);

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
  }
  return 0;
}
