#include "utils.h"

typedef struct{
  float x;
  float y; 
  unsigned char mask[3];
} Vertex;

typedef struct{
  f32_array vertices;
  u8_array indices_s;  // solid
  u8_array indices_w;  // wireframe
} Shape;

GLuint ui_left_VAO, ui_left_VBO, ui_left_EBO;
GLuint ui_right_VAO, ui_right_VBO, ui_right_EBO;

mat4 uProjection;
GLuint program_ui, uProjectionLoc, uColorUILoc;
int SCREEN_WIDTH;
int SCREEN_HEIGHT;

extern const float vertices_shapes[];
extern const unsigned char indices_w_shapes[];
extern const unsigned char indices_s_shapes[];

Shape generate_tetragon(float center_x, float center_y, float size){
  Shape result = {};
  
  init_f32_array(&(result.vertices), (float[]){ 
    center_x - size * 0.5, center_y - size * 0.5,
    center_x + size * 0.5, center_y - size * 0.5,
    center_x + size * 0.5, center_y + size * 0.5,
    center_x - size * 0.5, center_y + size * 0.5
  }, 8);

  init_u8_array(&(result.indices_w), (unsigned char[]){
    0, 1, 1, 2, 2, 3, 3, 0
  }, 8);

  init_u8_array(&(result.indices_s), (unsigned char[]){
    0, 1, 3, 2, 1, 3
  }, 6);

  return result;
};

Shape generate_trigon(float center_x, float center_y, float size){
  Shape result = {};
  
  init_f32_array(&(result.vertices), (float[]){ 
    center_x - size * 0.5, center_y - size * 0.4,
    center_x + size * 0.5, center_y - size * 0.4,
    center_x,              center_y + size * 0.4
  }, 6);

  init_u8_array(&(result.indices_w), (unsigned char[]){
    0, 1, 1, 2, 2, 0
  }, 6);

  init_u8_array(&(result.indices_s), (unsigned char[]){
    0, 1, 2
  }, 3);

  return result;
}

Shape generate_hexagon(float center_x, float center_y, float size){
  Shape result = {};
  
  init_f32_array(&(result.vertices), (float[]){
    center_x,              center_y - size * 0.5,
    center_x + size * 0.5, center_y - size * 0.25,
    center_x + size * 0.5, center_y + size * 0.25,
    center_x,              center_y + size * 0.5,
    center_x - size * 0.5, center_y + size * 0.25,
    center_x - size * 0.5, center_y - size * 0.25,
  }, 12);

  init_u8_array(&(result.indices_w), (unsigned char[]){
    0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 0
  }, 12);

  init_u8_array(&(result.indices_s), (unsigned char[]){
    0, 1, 3, 1, 2, 3, 0, 3, 4, 4, 5, 0
  }, 12);

  return result;
}

Shape generate_run_sign(float center_x, float center_y, float size){
  Shape result = {};
  
  init_f32_array(&(result.vertices), (float[]){
    center_x - size * 0.4, center_y + size * 0.3,
    center_x + size * 0.4, center_y,
    center_x - size * 0.4, center_y - size * 0.3
  }, 6);

  init_u8_array(&(result.indices_w), (unsigned char[]){
    0, 1, 1, 2, 2, 0
  }, 6);

  init_u8_array(&(result.indices_s), (unsigned char[]){
    0, 1, 2
  }, 3);

  return result;
}

unsigned int indices_left_w[32] = {};
unsigned int indices_left_s[24] = {};
unsigned int indices_right_w[104] = {};
unsigned int indices_right_s[200] = {};

void add_shape(
  int* index_w, int* count_w,
  int* index_s, int* count_s,
  int* index_v,
  Shape source,
  unsigned char r, unsigned char g, unsigned char b,
  unsigned int* target_w,
  unsigned int* target_s,
  Vertex* target_v
){
  for(int j=0; j<source.indices_w.size; j++)
    target_w[(*index_w)++] = (*count_w) + source.indices_w.data[j];
  (*count_w) += source.vertices.size * 0.5;

  for(int j=0; j<source.indices_s.size; j++)
    target_s[(*index_s)++] = (*count_s) + source.indices_s.data[j];
  (*count_s) += source.vertices.size * 0.5;

  for(int j=0; j<source.vertices.size; j+=2){
    target_v[(*index_v)++] = (Vertex){
      source.vertices.data[j],
      source.vertices.data[j+1],
      {r, g, b}};
  }
}

void ui_init(GLuint program, int width, int height){
  #define GENERATE_TRIGON(height_modifier, size_modifier) \
    generate_trigon(padding, SCREEN_HEIGHT - SCREEN_HEIGHT * \
      height_modifier, size * size_modifier);;
  #define GENERATE_TETRAGON(height_modifier, size_modifier) \
    generate_tetragon(padding, SCREEN_HEIGHT - SCREEN_HEIGHT * \
      height_modifier, size * size_modifier);;
  #define GENERATE_HEXAGON(height_modifier, size_modifier) \
    generate_hexagon(padding, SCREEN_HEIGHT - SCREEN_HEIGHT * \
      height_modifier, size * size_modifier);;

  SCREEN_WIDTH = width;
  SCREEN_HEIGHT = height;
  program_ui = program;
  
  glGenVertexArrays(1, &ui_left_VAO);
  glGenBuffers(1, &ui_left_VBO);
  glGenBuffers(1, &ui_left_EBO);
  glGenVertexArrays(1, &ui_right_VAO);
  glGenBuffers(1, &ui_right_VBO);
  glGenBuffers(1, &ui_right_EBO);

  float size = SCREEN_WIDTH * 0.1 - SCREEN_WIDTH * 0.015 * 2;
  float padding = SCREEN_WIDTH * 0.05;

  Shape trigon =   GENERATE_TRIGON  (0.1, 1.0);
  Shape tetragon = GENERATE_TETRAGON(0.3, 0.85);
  Shape hexagon =  GENERATE_HEXAGON (0.5, 0.95);
  Shape run_sign = generate_run_sign(padding,
    SCREEN_HEIGHT * 0.1, size);

  Vertex vertices_left[16] = {};
  {
    #define ADD_SHAPE_LEFT(type, r, g, b) \
      add_shape(&iw, &cw, &is, &cs, &iv, \
        type, r, g, b, \
        indices_left_w,  indices_left_s, vertices_left);

    int iw = 0, cw = 0, is = 0, cs = 0, iv = 0;
    ADD_SHAPE_LEFT(trigon,   255, 0,   0);
    ADD_SHAPE_LEFT(tetragon, 255, 255, 0);
    ADD_SHAPE_LEFT(hexagon,  255, 0,   255);
    ADD_SHAPE_LEFT(run_sign, 0,   255, 255);
  }

  Shape trigon_l =  GENERATE_TRIGON(0.2, 1.0);
  Shape trigon_m =  GENERATE_TRIGON(0.4, 0.75);
  Shape trigon_s =  GENERATE_TRIGON(0.6, 0.5);
  Shape trigon_xs = GENERATE_TRIGON(0.8, 0.25);

  Shape tetragon_l =  GENERATE_TETRAGON(0.2, 0.85);
  Shape tetragon_m =  GENERATE_TETRAGON(0.4, 0.6375);
  Shape tetragon_s =  GENERATE_TETRAGON(0.6, 0.425);
  Shape tetragon_xs = GENERATE_TETRAGON(0.8, 0.2125);

  Shape hexagon_l =  GENERATE_HEXAGON(0.2, 0.95);
  Shape hexagon_m =  GENERATE_HEXAGON(0.4, 0.7125);
  Shape hexagon_s =  GENERATE_HEXAGON(0.6, 0.475);
  Shape hexagon_xs = GENERATE_HEXAGON(0.8, 0.2375);

  Vertex vertices_right[52] = {};
  { 
    #define ADD_SHAPE_RIGHT(type, r, g, b) \
      add_shape(&iw, &cw, &is, &cs, &iv, \
        type, r, g, b, \
        indices_right_w, indices_right_s, vertices_right);

    int iw = 0, cw = 0, is = 0, cs = 0, iv = 0;
    ADD_SHAPE_RIGHT(trigon_l,  255, 0,   0);
    ADD_SHAPE_RIGHT(trigon_m,  255, 255, 0);
    ADD_SHAPE_RIGHT(trigon_s,  255, 0,   255);
    ADD_SHAPE_RIGHT(trigon_xs, 0,   255, 255);
    
    ADD_SHAPE_RIGHT(tetragon_l,  255, 0,   0);
    ADD_SHAPE_RIGHT(tetragon_m,  255, 255, 0);
    ADD_SHAPE_RIGHT(tetragon_s,  255, 0,   255);
    ADD_SHAPE_RIGHT(tetragon_xs, 0,   255, 255);
    
    ADD_SHAPE_RIGHT(hexagon_l,  255, 0,   0);
    ADD_SHAPE_RIGHT(hexagon_m,  255, 255, 0);
    ADD_SHAPE_RIGHT(hexagon_s,  255, 0,   255);
    ADD_SHAPE_RIGHT(hexagon_xs, 0,   255, 255);
  }

  glBindVertexArray(ui_left_VAO);

  glBindBuffer(GL_ARRAY_BUFFER, ui_left_VBO);
  glBufferData(
    GL_ARRAY_BUFFER,
    sizeof(vertices_left),
    vertices_left,
    GL_STATIC_DRAW);
  glVertexAttribPointer(
    0, 
    2, 
    GL_FLOAT, 
    GL_FALSE,sizeof(Vertex), 
    (void*)offsetof(Vertex, x));
  
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(
    1, 
    3, 
    GL_UNSIGNED_BYTE, 
    GL_TRUE, 
    sizeof(Vertex), 
    (void*)offsetof(Vertex, mask));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);

  glBindVertexArray(ui_right_VAO);

  glBindBuffer(GL_ARRAY_BUFFER, ui_right_VBO);
  glBufferData(
    GL_ARRAY_BUFFER, 
    sizeof(vertices_right), 
    vertices_right, 
    GL_STATIC_DRAW);
  glVertexAttribPointer(
    0, 
    2, 
    GL_FLOAT, 
    GL_FALSE,sizeof(Vertex), 
    (void*)offsetof(Vertex, x));
  
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(
    1, 
    3, 
    GL_UNSIGNED_BYTE, 
    GL_TRUE, 
    sizeof(Vertex), 
    (void*)offsetof(Vertex, mask));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);

  glm_ortho(
    0.0f, SCREEN_WIDTH * 0.1, 
    0.0f, SCREEN_HEIGHT, 
    0.0f, 1.0f, 
    uProjection);

  uProjectionLoc = glGetUniformLocation(program, "uProjection");
  uColorUILoc = glGetUniformLocation(program, "uColor");
}

void ui_left_render(Mode mode, int play){
  glEnable(GL_SCISSOR_TEST);
  glViewport(0, 0, SCREEN_WIDTH * 0.1, SCREEN_HEIGHT);
  glScissor(0, 0, SCREEN_WIDTH * 0.1, SCREEN_HEIGHT);

  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glUniform3f(uColorUILoc, 1.0, 1.0, 1.0);

  glUniformMatrix4fv(
    uProjectionLoc,
    1,
    GL_FALSE,
    (const GLfloat*)uProjection);
  glBindVertexArray(ui_left_VAO);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_left_EBO);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER, 
    sizeof(indices_left_w), 
    indices_left_w, 
    GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDrawElements(GL_LINES, 32, GL_UNSIGNED_INT, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_left_EBO);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER, 
    sizeof(indices_left_s), 
    indices_left_s, 
    GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  switch (mode){
    case TRIGON:{
      // draw 3 vertices without offset
      glDrawElements(
        GL_TRIANGLES, 
        3, 
        GL_UNSIGNED_INT, 
        (void*)0);
      break;
    }
    case HEXAGON:{
      // draw 6 vertices with offset (starting from) = vertices
      // of trigon and tetragon
      glDrawElements(
        GL_TRIANGLES, 
        12, 
        GL_UNSIGNED_INT, 
        (void*)(9 * sizeof(unsigned int)));
      break;
    }
    case TETRAGON:{
      // draw 4 vertices with offset (starting from) = vertices
      // of trigon
      glDrawElements(
        GL_TRIANGLES, 
        6, 
        GL_UNSIGNED_INT, 
        (void*)(3 * sizeof(unsigned int)));
      break;
    }
  }

  if(play)
    glDrawElements(
      GL_TRIANGLES, 
      3, 
      GL_UNSIGNED_INT, 
      (void*)(21 * sizeof(unsigned int)));
  
  glDisable(GL_SCISSOR_TEST);
}

void ui_right_render(Mode mode, Size size){
  glEnable(GL_SCISSOR_TEST);
  glViewport(SCREEN_WIDTH * 0.9, 0, SCREEN_WIDTH * 0.1, SCREEN_HEIGHT);
  glScissor(SCREEN_WIDTH * 0.9, 0, SCREEN_WIDTH * 0.1, SCREEN_HEIGHT);

  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glUniform3f(uColorUILoc, 1.0, 1.0, 1.0);

  glUniformMatrix4fv(
    uProjectionLoc,
    1,
    GL_FALSE,
    (const GLfloat*)uProjection);
  glBindVertexArray(ui_right_VAO);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_right_EBO);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER, 
    sizeof(indices_right_w), 
    indices_right_w, 
    GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  switch (mode){
    case TRIGON:{
      // draw 3 vertices without offset
      glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);

      if(size != -1){
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_right_EBO);
        glBufferData(
          GL_ELEMENT_ARRAY_BUFFER, 
          sizeof(indices_right_s), 
          indices_right_s, 
          GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        unsigned long offset = 0;
        switch (size){
          case L:{ offset = 0; break; }
          case M:{ offset = 3 * sizeof(unsigned int); break; }
          case S:{ offset = 6 * sizeof(unsigned int); break; }
          case XS:{ offset = 9 * sizeof(unsigned int); break; }
        }
      
        glDrawElements(
          GL_TRIANGLES, 
          3, 
          GL_UNSIGNED_INT, 
          (void*) offset);
      }
      break;
    }
    case HEXAGON:{
      // draw 6 vertices with offset (starting from) = vertices
      // of trigon and tetragon
      glDrawElements(
        GL_LINES,
        48,
        GL_UNSIGNED_INT,
        (void*)(56 * sizeof(unsigned int)));

      if(size != -1){
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_right_EBO);
        glBufferData(
          GL_ELEMENT_ARRAY_BUFFER,
          sizeof(indices_right_s),
          indices_right_s,
          GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        unsigned long offset = 0;
        switch (size){
          case L:{ offset = 36 * sizeof(unsigned int); break; }
          case M:{ offset = 48 * sizeof(unsigned int); break; }
          case S:{ offset = 60 * sizeof(unsigned int); break; }
          case XS:{ offset = 72 * sizeof(unsigned int); break; }
        }
        glDrawElements(
          GL_TRIANGLES, 
          12, 
          GL_UNSIGNED_INT, 
          (void*) offset);
      }
      break;
    }
    case TETRAGON:{
      // draw 4 vertices with offset (starting from) = vertices
      // of trigon
      glDrawElements(
        GL_LINES,
        32,
        GL_UNSIGNED_INT,
        (void*)(24 * sizeof(unsigned int)));
      
      if(size != -1){
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_right_EBO);
        glBufferData(
          GL_ELEMENT_ARRAY_BUFFER, 
          sizeof(indices_right_s), 
          indices_right_s, 
          GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        unsigned long offset = 0;
        switch (size){
          case L:{ offset = 12 * sizeof(unsigned int); break; }
          case M:{ offset = 18 * sizeof(unsigned int); break; }
          case S:{ offset = 24 * sizeof(unsigned int); break; }
          case XS:{ offset = 30 * sizeof(unsigned int); break; }
        }
        glDrawElements(
          GL_TRIANGLES, 
          6, 
          GL_UNSIGNED_INT, 
          (void*) offset);
      }
      break;
    }
  }

  glDisable(GL_SCISSOR_TEST);
}

void ui_left_click(int x, int y, Mode* mode, int* play){
  glEnable(GL_SCISSOR_TEST);
  glViewport(0, 0, SCREEN_WIDTH * 0.1, SCREEN_HEIGHT);
  glScissor(0, 0, SCREEN_WIDTH * 0.1, SCREEN_HEIGHT);

  glUseProgram(program_ui);

  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glUniform3f(uColorUILoc, 0.0, 0.0, 0.0);

  glBindVertexArray(ui_left_VAO);
  glUniformMatrix4fv(
    uProjectionLoc,
    1,
    GL_FALSE,
    (const GLfloat*)uProjection);
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_left_EBO);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER, 
    sizeof(indices_left_s), 
    indices_left_s, 
    GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDrawElements(
    GL_TRIANGLES,
    24,
    GL_UNSIGNED_INT,
    0);
  
  glDisable(GL_SCISSOR_TEST);

  unsigned char pixel[3];
  glReadPixels(
    x, y,
    1, 1,
    GL_RGB,
    GL_UNSIGNED_BYTE,
    pixel);

  if(pixel[0] == 255 && pixel[1] == 0 && pixel[2] == 0)
    *mode = TRIGON;
  if(pixel[0] == 255 && pixel[1] == 255 && pixel[2] == 0)
    *mode = TETRAGON;
  if(pixel[0] == 255 && pixel[1] == 0 && pixel[2] == 255)
    *mode = HEXAGON;
  if(pixel[0] == 0 && pixel[1] == 255 && pixel[2] == 255)
    *play = (*play) > 0 ? 0 : 1;
}

void ui_right_click(int x, int y, Mode mode, Size* size){
  glEnable(GL_SCISSOR_TEST);
  glViewport(SCREEN_WIDTH * 0.9, 0, SCREEN_WIDTH * 0.1, SCREEN_HEIGHT);
  glScissor(SCREEN_WIDTH * 0.9, 0, SCREEN_WIDTH * 0.1, SCREEN_HEIGHT);

  glUseProgram(program_ui);

  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glUniform3f(uColorUILoc, 0.0, 0.0, 0.0);

  glBindVertexArray(ui_right_VAO);
  glUniformMatrix4fv(
    uProjectionLoc, 
    1, 
    GL_FALSE, 
    (const GLfloat*)uProjection);
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_right_EBO);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER, 
    sizeof(indices_right_s), 
    indices_right_s, 
    GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  switch (mode){
    case TRIGON:{
      glDrawElements(
        GL_TRIANGLES, 
        12, 
        GL_UNSIGNED_INT, 
        0);
      break;
    }
      case HEXAGON:{
      glDrawElements(
        GL_TRIANGLES, 
        24, 
        GL_UNSIGNED_INT, 
        (void*)(12 * sizeof(unsigned int)));
      break;
    }
    case TETRAGON:{
      glDrawElements(
        GL_TRIANGLES, 
        48, 
        GL_UNSIGNED_INT, 
        (void*)(36 * sizeof(unsigned int)));
      break;
    }
  }  
  glDisable(GL_SCISSOR_TEST);

  unsigned char pixel[3];
  glReadPixels(
    x, y, 
    1, 1, 
    GL_RGB, 
    GL_UNSIGNED_BYTE, 
    pixel);

  if(pixel[0] == 255 && pixel[1] == 0 && pixel[2] == 0)
    *size = L;
  if(pixel[0] == 255 && pixel[1] == 255 && pixel[2] == 0)
    *size = M;
  if(pixel[0] == 255 && pixel[1] == 0 && pixel[2] == 255)
    *size = S;
  if(pixel[0] == 0 && pixel[1] == 255 && pixel[2] == 255)
    *size = XS;
}

void ui_destroy(){
  glDeleteVertexArrays(1, &ui_left_VAO);
  glDeleteBuffers(1, &ui_left_VBO);
  glDeleteBuffers(1, &ui_left_EBO);
  
  glDeleteVertexArrays(1, &ui_right_VAO);
  glDeleteBuffers(1, &ui_right_VBO);
  glDeleteBuffers(1, &ui_right_EBO);
}
