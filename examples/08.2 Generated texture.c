#define GL_GLEXT_PROTOTYPES
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>
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

f32 vertices[52] = {
  // Square
  //Vertices        //Texture
 -0.250f, 0.250f,   0.0f, 0.00f,
  0.250f, 0.250f,   1.0f, 0.00f,
  0.250f,-0.250f,   1.0f, 1.00f,
 -0.250f,-0.250f,   0.0f, 1.00f,
  // Triangle
 -0.250f, 0.250f,   0.0f, 0.00f,
  0.250f, 0.250f,   1.0f, 0.00f,
  0.000f,-0.250f,   0.5f, 1.00f,
  // Hex
 -0.217f, 0.124f,   0.0f, 0.75f,
  0.000f, 0.250f,   0.5f, 1.00f,
  0.217f, 0.124f,   1.0f, 0.75f,
  0.217f,-0.124f,   1.0f, 0.25f,
  0.000f,-0.250f,   0.5f, 0.00f,
 -0.217f,-0.124f,   0.0f, 0.25f
};
u8 indices_w[26] = {
  0, 1, 1, 2, 2, 3, 3, 0,
  4, 5, 5, 6, 6, 4,
  7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 7, 
};
u8 indices_s[21] = {
  0, 1, 2, 0, 2, 3,
  4, 5, 6,
  7, 8, 9, 7, 9, 10, 7, 12, 10, 10, 11, 12 
};

f32 instancePositionAndTypes[9] = {
//Position      // Type    
 -0.6f, 0.0f,   0.0f, // Square
  0.0f, 0.0f,   1.0f, // Trangle
  0.6f, 0.0f,   2.0f  // Hex
};

f32 instanceStates[3] = {
  1.0,
  0.0,
  0.0 
};

const char* vertexShaderSource = 
  "#version 330 core                           \n"

  "layout(location = 0) in vec2 aPos;          \n"
  "layout(location = 1) in vec2 aTex;          \n"
  "layout(location = 2) in vec3 iPos;          \n"
  "layout(location = 3) in float iState;       \n"

  "uniform float uState;                       \n"

  "out float fragState;                        \n"
  "out vec2 texCoord;                          \n"

  "void main() {                               \n"
  "  float dx = 0;                             \n"

  /*
    - for glDrawArrays(...)
      gl_VertexID will take values 0..n for the vertices
      starts from 0
    - for glDrawElements(...)
      gl_VertexID will correspond to the indices in the element array buffer
      depends on the content of buffer
  */

  "  if(iPos.z == 0){                          \n"
  "    if(gl_VertexID > 3)                     \n"
  "      dx = -10000.0;                        \n"
  "  } else if (iPos.z == 1){                  \n"
  "    if(gl_VertexID < 4 || gl_VertexID > 6)  \n"
  "      dx = -10000.0;                        \n"
  "  } else {                                  \n"
  "    if(gl_VertexID < 7)                     \n"
  "      dx = -10000.0;                        \n"
  "  }                                         \n"

  "  if((iState + uState) < 1.0)               \n"
  "      dx = -10000.0;                        \n"
  
  "  gl_Position =                             \n"
  "    vec4(                                   \n"
  "     aPos.x + iPos.x + dx,                  \n"
  "     aPos.y + iPos.y,                       \n"
  "     0.0,                                   \n"
  "     1.0);                                  \n"

  "  fragState = uState > 0 ? 0.0 : iState;    \n"
  "  texCoord = vec2(aTex.x, 1.0 - aTex.y);    \n"
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

void APIENTRY debugCallback(
  GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  GLsizei length,
  const GLchar *message,
  const void *userParam) {
    printf("Debug message: %s\n", message);

    /*
  Statistics about shader's resource usage and performance characteristics on the GPU  
  Example:
  
  Debug message: Shader Stats: SGPRS: 16 VGPRS: 20 Code Size: 392
                 LDS: 0 Scratch: 0 Max Waves: 10 Spilled SGPRs: 0
                 Spilled VGPRs: 0 PrivMem VGPRs: 0

  How to understand it:
  1. SGPRS: 16
    Scalar General Purpose Registers (SGPRs):
      These are scalar registers used for operations where all threads
      in a wave execute the same operation with the same value.
      SGPRs are typically used for uniform values (like constants, uniforms,
      or other global parameters).
    16 SGPRs means your shader uses 16 scalar registers.
  2. VGPRS: 20
    Vector General Purpose Registers (VGPRs):
      VGPRs store per-thread data, like attributes or intermediate
      calculations that vary between threads.
      Each thread in a wave (or workgroup) gets its own set of VGPRs.
    20 VGPRs indicates that each thread requires 20 vector registers.
  3. Code Size: 392
    The size of the compiled shader program in bytes.
    A larger code size might increase the time to load the shader
    but generally isn’t an issue unless it's excessively large.
  4. LDS: 0
    Local Data Share (LDS):
      Shared memory available to threads in the same workgroup for
      fast communication.
    0 LDS means the shader doesn't use any shared local memory.
  5. Scratch: 0
    Scratch Memory:
      Temporary memory used when the shader spills (exceeds available registers).
    0 Scratch indicates the shader doesn’t spill into slower scratch memory,
      which is good for performance.
  6. Max Waves: 10
    Maximum Waves Per Compute Unit:
      A wave (or warp on NVIDIA GPUs) is a group of threads executed
      together by the GPU.
      The Max Waves value represents the theoretical maximum number of
      concurrent waves that can run on a compute unit.
    10 waves is a good balance, but fewer waves reduce parallelism, while too
      many waves may reduce available VGPRs/SGPRs per thread.
  7. Spilled SGPRs: 0
    Indicates how many SGPRs spilled into slower memory due to insufficient register space.
    0 Spilled SGPRs is ideal, meaning no scalar register values were moved to memory.
  8. Spilled VGPRs: 0
    Similar to SGPR spills, but for vector registers.
    0 Spilled VGPRs means no vector register values were spilled into slower memory,
      ensuring better performance.
  9. PrivMem VGPRs: 0
    Private Memory VGPRs:
      Memory allocated per-thread for private storage.
    0 PrivMem VGPRs means your shader doesn't use additional private memory,
      which is a positive sign for performance.

  Explanation:
  1. Efficient Register Usage:
    Low or moderate usage of SGPRs (16) and VGPRs (20) is good, as it allows more
    waves to execute in parallel on the GPU.
  2. No Register Spills:
    No spilled SGPRs or VGPRs is a great sign, as spills can drastically reduce
    performance by introducing memory latency.
  3. Compact Shader Code:
    A code size of 392 bytes is relatively small, meaning the shader is compact
    and will load quickly.
  4.Good Parallelism:
    The shader can execute up to 10 waves, indicating good utilization of GPU compute units.

  How to use this information if you're optimizing your shader:
  - Reduce VGPR or SGPR usage: If you’re close to hardware limits,
    consider optimizing by reducing register usage (e.g., by reusing variables or
    simplifying computations).
  - Avoid Spills: Spills into scratch memory can significantly slow down performance.
  - Keep Code Compact: Larger shaders might exceed limits on some GPUs.
  - Profile Further: Use tools like RenderDoc or Nsight Graphics to correlate
    these stats with your shader’s performance.
*/
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

    // glEnable(GL_DEBUG_OUTPUT);
    // glDebugMessageCallback(debugCallback, NULL);
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
      2, // only x,y are vertices actual coords
      GL_FLOAT,
      GL_FALSE,
      4 * sizeof(float), // stride as we are using x,y,s,t for each vertex  
      (void*)0);
    glEnableVertexAttribArray(0);
    // Not able to pass everything as vec4(x,y,s,t) with EBO
    // split to not mess indices 
    glVertexAttribPointer(
      1,               
      2, // s,t per vertex              
      GL_FLOAT,        
      GL_FALSE,        
      4 * sizeof(float), // -||-
      (void*)(2 * sizeof(float)) // offset for skip x,y 
    );
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
    glBufferData(
      GL_ARRAY_BUFFER,
      sizeof(instancePositionAndTypes),
      instancePositionAndTypes,
      GL_STATIC_DRAW);
    glVertexAttribPointer(
      2,
      3,
      GL_FLOAT,
      GL_FALSE,
      3 * sizeof(float),
      (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glBindBuffer(GL_ARRAY_BUFFER, stateVBO);
    glBufferData(
      GL_ARRAY_BUFFER,
      sizeof(instanceStates),
      instanceStates,
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
  
  GLuint uStateLoc, uTextureLoc;
  {
    uStateLoc = glGetUniformLocation(shaderProgram, "uState");
    uTextureLoc = glGetUniformLocation(shaderProgram, "uTexture");
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
    u8 spacing = 2;
    
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

    glUseProgram(shaderProgram);

    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(uTextureLoc, 0);

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
      3);

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
      3);

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
