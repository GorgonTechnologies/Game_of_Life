#include "utils.h"

mat4 uProjGame;
GLuint program_game;
GLuint uProjectionLoc, uColorLoc, uStateLoc, uShapeLoc, uSamplerLoc;
GLuint VAO, VBO, EBO, seedVBO;
GLuint grid_texture;

int SCREEN_WIDTH;
int SCREEN_HEIGHT;

int ROWS = 0;    int COLUMNS = 0;
int padding = 0;

f32_array vertices = {};
u8_array indices_s = {};  // solid
u8_array indices_w = {};  // wireframe
Grid seed = { .mode = HEXAGON, .generation = 0 };

void neighbors_indices_trigon(int row, int col, Grid in, int out[12]){
  int i = 0;
  if(col + 1 < in.cols)
    out[i++] = row * in.cols + col + 1; 
  if(col - 1 >= 0) 
    out[i++] = row * in.cols + col - 1;
  if(col + 1 < in.cols && row + 1 < in.rows) 
    out[i++] = (row + 1) * in.cols + col + 1;
  if(row + 1 < in.rows) 
    out[i++] = (row + 1) * in.cols + col;
  if(col - 1 >= 0 && row + 1 < in.rows) 
    out[i++] = (row + 1) * in.cols + col - 1;
  if(col + 1 < in.cols && row - 1 >=0) 
    out[i++] = (row - 1) * in.cols + col + 1;
  if(row - 1 >= 0) 
    out[i++] = (row - 1) * in.cols + col;
  if(col - 1 >= 0 && row - 1 >= 0) 
    out[i++] = (row - 1) * in.cols + col - 1;
  if(col + 2 < in.cols) 
    out[i++] = row * in.cols + col + 2;
  if(col - 2 >= 0) 
    out[i++] = row * in.cols + col - 2;
  
  if(in.data[row * in.cols + col].flip > 0){
    if(col - 2 >= 0 && row - 1 >= 0)
        out[i++] = (row - 1) * in.cols + col - 2;
    if(col + 2 < in.cols && row - 1 >= 0)
        out[i++] = (row - 1) * in.cols + col + 2;
  } else{
    if(col - 2 >= 0 && row + 1 < in.rows)
        out[i++] = (row + 1) * in.cols + col - 2;
    if(col + 2 < in.cols && row + 1 < in.rows)
        out[i++] = (row + 1) * in.cols + col + 2;
  }
}

void neighbors_indices_hexagon(int row, int col, Grid in, int out[12]){
  int i = 0;
  if(col + 1 < in.cols)
    out[i++] = row * in.cols + col + 1; 
  if(col - 1 >= 0) 
    out[i++] = row * in.cols + col - 1;
  if(row - 1 >= 0) 
    out[i++] = (row - 1) * in.cols + col;
  if(row + 1 < in.rows) 
    out[i++] = (row + 1) * in.cols + col;
  
  if(row%2){
    if(col + 1 < in.cols && row - 1 >=0) 
      out[i++] = (row - 1) * in.cols + col + 1;
    if(col + 1 < in.cols && row + 1 < in.rows) 
      out[i++] = (row + 1) * in.cols + col + 1;
  } else {
    if(col - 1 >= 0 && row + 1 < in.rows) 
      out[i++] = (row + 1) * in.cols + col - 1;
    if(col - 1 >= 0 && row - 1 >= 0) 
      out[i++] = (row - 1) * in.cols + col - 1;
  }
}

void neighbors_indices_tetragon(int row, int col, Grid in, int out[12]){
  int i = 0;
  if(col + 1 < in.cols) 
    out[i++] = row * in.cols + col + 1; 
  if(col - 1 >= 0)
    out[i++] = row * in.cols + col - 1;
  if(row - 1 >= 0)
    out[i++] = (row - 1) * in.cols + col;
  if(row + 1 < in.rows)
    out[i++] = (row + 1) * in.cols + col;
  if(col + 1 < in.cols && row + 1 < in.rows)
    out[i++] = (row + 1) * in.cols + col + 1;
  if(col - 1 >= 0 && row + 1 < in.rows)
    out[i++] = (row + 1) * in.cols + col - 1;
  if(col + 1 < in.cols && row - 1 >=0)
    out[i++] = (row - 1) * in.cols + col + 1;
  if(col - 1 >= 0 && row - 1 >= 0)
    out[i++] = (row - 1) * in.cols + col - 1;
}

void neighbors_indices(int row, int col, Grid in, int out[12]){
  switch (in.mode){
    case TRIGON: {
      neighbors_indices_trigon(row, col, seed, out);
      break;
    }
    case HEXAGON: {
      neighbors_indices_hexagon(row, col, seed, out);
      break;
    }
    case TETRAGON:
    default:{
      neighbors_indices_tetragon(row, col, seed, out);
      break;
    }
  }
}

/*
  Rules:
  - for tetragon cells - there is 8 neighbors:
    (horizontal - vertical - respective diagonals)
  - for each step:
    - any live cell with fewer than two live neighbors dies,
      as if by underpopulation
    - any live cell with more than three live neighbors dies,
      as if by overpopulation
    - any live cell with two or three live neighbors lives on
      the next generation
    - any dead cell with exactly three live neighbors becomes
      a live crll, as if by reproduction
  - initial pattern constitutes the seed of te system 

  -- next_generation(2,3,3)
*/
char neighbors_alive(int row, int col, Grid in){
  char value = 0; 
  int indices[12] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

  neighbors_indices(row, col, in, indices);

  for(int i=0; i<12; i++)
    if(indices[i] > -1 && in.data[indices[i]].state == 1)
      value++;

  return value;
}

char neighbors_alive_index(int index, Grid in){
  int row = index / in.cols;
  int col = index % in.cols;
  return neighbors_alive(row, col, in);
}

/*
  Params:
  U - underpopulation
  O - overpopulation
  R - reproduction

  ( S - survival :: U <= n <= O )
*/
void next_generation(char u, char o, char r){
  char** next = calloc(sizeof(char*), seed.rows);
  for(int i=0; i< seed.rows; i++)
    next[i] = calloc(sizeof(char), seed.cols);

  for(int i = 0; i< seed.rows; i++)
    for(int j = 0; j< seed.cols; j++){
      char n = neighbors_alive(i, j, seed);
      if(n < u) next[i][j] = 0;
      if(n > o) next[i][j] = 0;
      if(n >= u && n <= o) next[i][j] =
        seed.data[i * seed.cols + j].state;
      if(n == r) next[i][j] = 1;
    }

  for(int i = 0; i< seed.rows; i++)
    for(int j = 0; j< seed.cols; j++){
      seed.data[i * seed.cols + j].state = next[i][j];
    }
  seed.generation++;

  glBindBuffer(GL_ARRAY_BUFFER, seedVBO);
  glBufferSubData(
    GL_ARRAY_BUFFER,
    0,
    sizeof(Cell) * seed.rows * seed.cols,
    seed.data);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void game_init(GLuint program, Mode mode, Size size_, int width, int height){
  int size;
  switch(size_){
    case XS:{ size = 24; break; }
    case S:{ size = 48; break; }
    case M:{ size = 72; break; }
    case L:{ size = 96; break; }
    default: return;
  }
  seed.mode = mode;

  SCREEN_WIDTH = width;
  SCREEN_HEIGHT = height;
  program_game = program;
  vec4 bounds = {5, 5, SCREEN_WIDTH*0.8-5, SCREEN_HEIGHT-5};

  switch (seed.mode) {
    case HEXAGON: {
      ROWS = (bounds[3] - bounds[1]) / (size * 0.8 + padding);
      COLUMNS = (bounds[2] - bounds[0]) / (size + padding);
      
      init_f32_array(&vertices, (float[]){
        0,           -size * 0.5,
        size * 0.5,  -size * 0.25,
        size * 0.5,   size * 0.25,
        0,            size * 0.5,
       -size * 0.5,   size * 0.25,
       -size * 0.5,  -size * 0.25,
      }, 12);

      init_u8_array(&indices_w, (unsigned char[]){
        0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 0
      }, 12);

      init_u8_array(&indices_s, (unsigned char[]){
        0, 1, 3, 1, 2, 3, 0, 3, 4, 4, 5, 0
      }, 12);

      seed.data = calloc(sizeof(Cell), ROWS * COLUMNS);
      seed.rows = ROWS;
      seed.cols = COLUMNS;

      for(int i=0;i<ROWS;i++)
        for(int j=0;j<COLUMNS;j++){
          seed.data[i * COLUMNS + j] =  (Cell){
            .x = bounds[0] + size * .5,
            .y = bounds[1] + size * .5,
            .flip=1.0
          };

          hash(i, j, seed.data[i * COLUMNS + j].color );

          if(j > 0) seed.data[i * COLUMNS + j].x = 
            seed.data[i * COLUMNS + j-1].x + size + padding;
          if(i > 0) {
            seed.data[i * COLUMNS + j].y = 
              seed.data[(i-1) * COLUMNS + j].y + size * 0.75 + padding * 0.8;
            if(i%2 && j==0)
              seed.data[i * COLUMNS + j].x = 
                seed.data[i * COLUMNS + j].x + size * 0.5 + padding * 0.5;
          }
        }
      break;
    }
    case TRIGON: {
      ROWS = (bounds[3] - bounds[1]) / (size * 0.8 + padding * 0.8);
      COLUMNS = (bounds[2] - bounds[0]) / (size * 0.5  + padding) - 1;

      init_f32_array(&vertices, (float[]){ 
       -size * 0.5, -size * 0.4,
        size * 0.5, -size * 0.4,
        0,           size * 0.4
      }, 6);

      init_u8_array(&indices_w, (unsigned char[]){
        0, 1, 1, 2, 2, 0
      }, 6);

      init_u8_array(&indices_s, (unsigned char[]){
        0, 1, 2
      }, 3);

      seed.data = calloc(sizeof(Cell), ROWS * COLUMNS);
      seed.rows = ROWS;
      seed.cols = COLUMNS;

      for(int i=0;i<ROWS;i++)
        for(int j=0;j<COLUMNS;j++){
          seed.data[i * COLUMNS + j] =  (Cell){
            .x = bounds[0] + size * .5,
            .y = bounds[1] + size * .5,
            .flip = 1.0
          };

          hash(i, j, seed.data[i * COLUMNS + j].color );

          if(j > 0){
            seed.data[i * COLUMNS + j].x = 
              seed.data[i * COLUMNS + j-1].x + size * 0.5 + padding;
            seed.data[i * COLUMNS + j].flip =
              -1.0 * seed.data[i * COLUMNS + j-1].flip;
          }
          if(i > 0) {
            seed.data[i * COLUMNS + j].y = 
              seed.data[(i-1) * COLUMNS + j].y + size * 0.8 + padding * 0.8;
            seed.data[i * COLUMNS + j].flip =
              -1.0 * seed.data[(i-1) * COLUMNS + j].flip;
          }
        }
      break;
    }
    case TETRAGON:
    default: { 
      ROWS = (bounds[3] - bounds[1]) / (size + padding);
      COLUMNS = (bounds[2] - bounds[0]) / (size + padding);

      init_f32_array(&vertices, (float[]){ 
       -size * 0.5, -size * 0.5,
        size * 0.5, -size * 0.5,
        size * 0.5,  size * 0.5,
       -size * 0.5,  size * 0.5
      }, 8);

      init_u8_array(&indices_w, (unsigned char[]){
        0, 1, 1, 2, 2, 3, 3, 0
      }, 8);

      init_u8_array(&indices_s, (unsigned char[]){
        0, 1, 3, 2, 1, 3
      }, 6);

      seed.data = calloc(sizeof(Cell), ROWS * COLUMNS);
      seed.rows = ROWS;
      seed.cols = COLUMNS;
      for(int i=0;i<ROWS;i++)
        for(int j=0;j<COLUMNS;j++){
          seed.data[i * COLUMNS + j] =  (Cell){
            .x = bounds[0] + size * .5,
            .y = bounds[1] + size * .5,
            .flip = 1.0
          };

          hash(i, j, seed.data[i * COLUMNS + j].color );

          if(j > 0) seed.data[i * COLUMNS + j].x = 
            seed.data[i * COLUMNS + j-1].x + size + padding;
          if(i > 0) seed.data[i * COLUMNS + j].y = 
            seed.data[(i-1) * COLUMNS + j].y + size + padding;
        }
      break;
    }
  }

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glGenBuffers(1, &seedVBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(
    GL_ARRAY_BUFFER,
    sizeof(float) * vertices.size,
    vertices.data,
    GL_STATIC_DRAW);
  glVertexAttribPointer(
    0,
    2,
    GL_FLOAT,
    GL_FALSE,
    2 * sizeof(float),
    (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, seedVBO);
  glBufferData(
    GL_ARRAY_BUFFER,
    sizeof(Cell) * seed.rows * seed.cols,
    seed.data,
    GL_DYNAMIC_DRAW);

  glVertexAttribPointer(
    2,
    3,
    GL_FLOAT,
    GL_FALSE,
    sizeof(Cell),
    (void*)offsetof(Cell,x));
  glEnableVertexAttribArray(2);
  glVertexAttribDivisor(2, 1);  

  glVertexAttribPointer(1,
    3,
    GL_UNSIGNED_BYTE,
    GL_TRUE,
    sizeof(Cell),
    (void*)offsetof(Cell, color));
  glEnableVertexAttribArray(1);
  glVertexAttribDivisor(1, 1); 

  glVertexAttribPointer(
    3,
    1,
    GL_FLOAT,
    GL_FALSE,
    sizeof(Cell),
    (void*)offsetof(Cell, state));
  glEnableVertexAttribArray(3);
  glVertexAttribDivisor(3, 1); 
  
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  glm_ortho(
    0.0f, SCREEN_WIDTH * 0.8,
    0.0f, SCREEN_HEIGHT,
    0.0f, 1.0f,
    uProjGame);

  uProjectionLoc = glGetUniformLocation(program_game, "uProjection");
  uColorLoc = glGetUniformLocation(program_game, "uColor");
  uStateLoc = glGetUniformLocation(program_game, "uState");
  uShapeLoc = glGetUniformLocation(program_game, "uShape");
  uSamplerLoc = glGetUniformLocation(program_game, "uGridTexture");

  grid_texture = create_grid_texture(1,1);
}

void game_click(int x, int y, Mode* mode){
  glEnable(GL_SCISSOR_TEST);
  glScissor(SCREEN_WIDTH * 0.1, 0, SCREEN_WIDTH * 0.8, SCREEN_HEIGHT);
  glViewport(SCREEN_WIDTH * 0.1, 0, SCREEN_WIDTH * 0.8, SCREEN_HEIGHT);

  glUniform3f(uColorLoc, 0.0, 0.0, 0.0);
  glUniform1f(uStateLoc, 1.0);

  // update EBO with reallocation as it has different size
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER, 
    sizeof(unsigned char) * indices_s.size, 
    indices_s.data, 
    GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glDrawElementsInstanced(
    GL_TRIANGLES, 
    vertices.size, 
    GL_UNSIGNED_BYTE, 
    0, 
    seed.rows * seed.cols);
                  
  unsigned char pixel[3];
  glReadPixels(
    x, y, 
    1, 1, 
    GL_RGB, 
    GL_UNSIGNED_BYTE, 
    pixel);
  if(pixel[0] != 255 && pixel[1] != 255 && pixel[2] != 255){
    unsigned int row_col[2] = {};
    rehash(pixel, row_col);

    int row = row_col[0];
    int col = row_col[1];
    
    int indices[12] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

    neighbors_indices(row, col, seed, indices);

    if(seed.data[row * seed.cols + col].state < 1.0){
      seed.data[row * seed.cols + col].state = 1.0;
      for(int i=0; i<12; i++)
        if(indices[i] > -1 && seed.data[indices[i]].state < 1.0)
          seed.data[indices[i]].state = 0.5;
    } else {
      if(!neighbors_alive(row, col, seed))
        seed.data[row * seed.cols + col].state = 0.0;
      else 
        seed.data[row * seed.cols + col].state = 0.5;
      for(int i=0; i<12; i++)
        if(indices[i] > -1 && seed.data[indices[i]].state != 1.0){
          if(!neighbors_alive_index(indices[i], seed))
            seed.data[indices[i]].state = 0.0;
          else
            seed.data[indices[i]].state = 0.5;
        }
    }

    // update VBO without reallocation as it has same size
    glBindBuffer(GL_ARRAY_BUFFER, seedVBO);
    glBufferSubData(
      GL_ARRAY_BUFFER,
      0,
      sizeof(Cell) * seed.rows * seed.cols,
      seed.data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }
  glDisable(GL_SCISSOR_TEST);
}

void game_render(Mode mode){
  glEnable(GL_SCISSOR_TEST);
  glScissor(SCREEN_WIDTH * 0.1, 0, SCREEN_WIDTH * 0.8, SCREEN_HEIGHT);
  glViewport(SCREEN_WIDTH * 0.1, 0, SCREEN_WIDTH * 0.8, SCREEN_HEIGHT);

  glUseProgram(program_game);

  switch (mode){
    case TETRAGON:{ glUniform1f(uShapeLoc, 0.0); break; }
    case TRIGON:{   glUniform1f(uShapeLoc, 1.0); break; }
    case HEXAGON:{  glUniform1f(uShapeLoc, 2.0); break; }
  }

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
        
  glUniformMatrix4fv(uProjectionLoc, 1, GL_FALSE, (const GLfloat*)uProjGame);
  glUniform3f(uColorLoc, 1.0, 1.0, 1.0);

  glBindVertexArray(VAO);

  glUniform1f(uStateLoc, -1.0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER, 
    sizeof(unsigned char) * indices_s.size, 
    indices_s.data, 
    GL_STATIC_DRAW);                
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, grid_texture);
  glUniform1i(uSamplerLoc, 0); 
  
  glDrawElementsInstanced(
    GL_TRIANGLES, 
    vertices.size, 
    GL_UNSIGNED_BYTE, 
    0, 
    seed.rows * seed.cols);

  glDisable(GL_BLEND);
  
  glUniform1f(uStateLoc, 1.0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER, 
    sizeof(unsigned char) * indices_w.size, 
    indices_w.data,
    GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDrawElementsInstanced(
    GL_LINES, 
    vertices.size, 
    GL_UNSIGNED_BYTE, 
    0, 
    seed.rows * seed.cols);

  glDisable(GL_SCISSOR_TEST);
}

void game_destroy(){
  free(seed.data);
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);
  glDeleteBuffers(1, &seedVBO);
}