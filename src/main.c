#include "utils.h"

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 600

void APIENTRY debugCallback(
  GLenum source, 
  GLenum type, 
  GLuint id, 
  GLenum severity, 
  GLsizei length, 
  const GLchar *message, 
  const void *userParam) {
  printf("Debug message: %s\n", message);
}

int main() {
  SDL_Window* window = NULL; 
  SDL_GLContext context = 0;
  if (!sdl_init(
    "Game of Life",
    &window,
    &context,
    SCREEN_WIDTH,
    SCREEN_HEIGHT)) {
      printf("Cannot init SDL\n");
      return -1;
  }

  extern const char* shader_ui_v;
  extern const char* shader_ui_f;
  extern const char* shader_game_v;
  extern const char* shader_game_f;

  GLuint program_ui = create_shader_program(shader_ui_v,  shader_ui_f);
  GLuint program_game = create_shader_program(shader_game_v,  shader_game_f);

  ui_init(program_ui, SCREEN_WIDTH, SCREEN_HEIGHT);

  // Create grid texture
  GLuint uColorLoc = glGetUniformLocation(program_ui, "uColor");

  Mode mode = -1;
  Size size = -1;

  int exit = 0;
  // Somewhy with only one render cycle it is flickering
  int update_ui_left = 2; 
  int update_ui_right = 2;

  //glEnable(GL_DEBUG_OUTPUT);
  //glDebugMessageCallback(debugCallback, NULL);

  int play = 0;
  Uint64 previousTime = SDL_GetTicks();
  do {
    if(play){
      Uint64 currentTime = SDL_GetTicks();

      if (currentTime - previousTime >= 1000) {
        next_generation(2,3,3);
        previousTime = currentTime;
      }
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        exit = 1;
      } else if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN){
        int x = event.button.x;
        int y = SCREEN_HEIGHT - event.button.y; // Flip Y

        if(x < SCREEN_WIDTH * 0.1){
          Mode prev = mode;
          ui_left_click(x, y, &mode,&play);
          update_ui_left = 2;
          if(mode != prev){
            update_ui_right = 2;
            game_destroy();
            game_init(program_game, mode, size, SCREEN_WIDTH, SCREEN_HEIGHT);
          }
        } else if(x < SCREEN_WIDTH * 0.9){
          game_click(x, y, &mode);
        } else {
          Size prev = size;
          ui_right_click(x, y, mode, &size);
          update_ui_right = 2;
          if(size != prev){
            game_destroy();
            game_init(program_game, mode, size, SCREEN_WIDTH, SCREEN_HEIGHT);
          }
        }
      }
    }

    if(update_ui_left + update_ui_right){
      glUseProgram(program_ui);
      glDisable(GL_BLEND);
      glUniform3f(uColorLoc, 1.0, 1.0, 1.0);

      if(update_ui_left){
        ui_left_render(mode, play);
        update_ui_left--;
      }
      if(update_ui_right){
        ui_right_render(mode, size);
        update_ui_right--;
      }
    }
    game_render(mode);
    
    SDL_GL_SwapWindow(window);
  } while (!exit);

  ui_destroy();
  game_destroy();

  glDeleteProgram(program_ui);
  glDeleteProgram(program_game);

  SDL_GL_DestroyContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
