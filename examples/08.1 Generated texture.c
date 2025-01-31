#define GL_GLEXT_PROTOTYPES
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

//gcc ./examples/main0.c -g -o main -lGL -lSDL3

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

float vertices[16] = {
  // Square
  //Vertices    //Texture
 -0.5f, 0.5f,   0.0f, 0.00f,
  0.5f, 0.5f,   1.0f, 0.00f,
  0.5f,-0.5f,   1.0f, 1.00f,
 -0.5f,-0.5f,   0.0f, 1.00f
};
char indices_s[21] = {
  0, 1, 2, 0, 2, 3 
};

const char* vertexShaderSource = 
  "#version 330 core                           \n"

  "layout(location = 0) in vec2 aPos;          \n"
  "layout(location = 1) in vec2 aTex;          \n"
  "out vec2 texCoord;                          \n"

  "void main() {                               \n"
  "  gl_Position = vec4(aPos, 0.0, 1.0);       \n"
  "  texCoord = aTex;                          \n"
  "}";

const char* fragmentShaderSource =
  "#version 330 core                           \n"

  "in vec2 texCoord;                           \n"
  "out vec4 fragColor;                         \n"
  "uniform sampler2D uTexture;                 \n"

  "void main() {                               \n"
  "  fragColor = texture(uTexture, texCoord);  \n"
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

#define chess_board_width 64
#define chess_board_height 64
static GLubyte chess_board_texture
  [chess_board_width]
  [chess_board_height]
  [4];

void create_chess_board_texture(void){
  for (unsigned char i = 0; i < chess_board_width; i++) {
    for (unsigned char j = 0; j < chess_board_height; j++) {
      unsigned char c = ((((i&0x8)==0)^((j&0x8))==0))*255;
      chess_board_texture[i][j][0] = (GLubyte) c;
      chess_board_texture[i][j][1] = (GLubyte) c;
      chess_board_texture[i][j][2] = (GLubyte) c;
      chess_board_texture[i][j][3] = (GLubyte) 255;
    }
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

  GLuint VAO, VBO, EBO;
  { 
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
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
      1,
      2,
      GL_FLOAT,
      GL_FALSE,
      4 * sizeof(float),
      (void*)(2 * sizeof(float))
    );
    
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
      GL_ELEMENT_ARRAY_BUFFER,
      sizeof(indices_s),
      indices_s,
      GL_STATIC_DRAW);
      
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
  
  GLuint uTextureLoc;
  {
    uTextureLoc = glGetUniformLocation(shaderProgram, "uTexture");
  }

  GLuint texture;
  {
    create_chess_board_texture();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA,
      chess_board_width,
      chess_board_height,
      0,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      chess_board_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE0);

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
      
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);

    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(uTextureLoc, 0);

    glBindVertexArray(VAO);
      
    glDrawElements(
      GL_TRIANGLES,
      8,
      GL_UNSIGNED_BYTE,
      0);

    SDL_GL_SwapWindow(window);
  }

  {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
  }
  return 0;
}
