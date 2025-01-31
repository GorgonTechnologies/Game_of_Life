#include "svg.h"

// gcc ./src/preprocessor.c -fsanitize=address -g -o ./build/preprocessor

void read_file(const char *filename, char **out) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("Failed to open file");
    return;
  }

  // fseek is moving the file pointer to the end and
  // ftell is getting the current position (file size)
  // rewind is reset the file pointer to the beginning of the file
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  rewind(file);

  char* buffer = (char *)malloc(fileSize + 1);
  if (!buffer) {
    perror("Failed to allocate memory");
    fclose(file);
    return;
  }

  size_t bytesRead = fread(buffer, 1, fileSize, file);
  // null terminator ('\0') at the end to make it a valid C-string
  buffer[bytesRead] = '\0'; 
  *out = buffer;
  fclose(file);
}

void write_shader(
  const char *output_file, 
  const char *variable_name, 
  const char *shader_file) {
    FILE *out = fopen(output_file, "a");
    if (!out) {
      perror("Failed to open output file");
      exit(EXIT_FAILURE);
    }
    char *buffer = NULL;
    char *token = NULL;

    read_file(shader_file, &buffer);

    fprintf(out, "const char* %s = ", variable_name);

    int finish = 0;
    do {
      finish = strtok_(&token, &buffer, 0, (Search_Until){
        .data = { "\n" }, 
        .size = 1});
      fprintf(out, "\n\"%s\\n\"", token);
    } while (!finish);
    
    fprintf(out, ";\n\n");

    // free(...); will be done by OS 
    fclose(out);
}

void write_svg(const char *output_file, const char *svg_file){
 FILE *out = fopen(output_file, "a");
  if (!out) {
    perror("Failed to open output file");
    exit(EXIT_FAILURE);
  }
  
  char* buffer = NULL;
  char* token = NULL;

  read_file(svg_file, &buffer);

  int finish = 0;
  do {
    // seek for the beginning of new tag
    finish = strtok_(&token, &buffer, '<', (Search_Until){
      .data = { "\n", " ", ">" }, 
      .size = 3});

    if(!strcmp(token,"?xml") // skip xml tags
      || !strcmp(token,"!--") // skip xml comments
      || !strcmp(token,"svg") // skip svg tags
      || !strcmp(token,"/svg")){ 
      continue;
    } else {
        if(!strcmp("g", token)){
          finish = strtok_(&token, &buffer, 0, (Search_Until){
            .data = { ">" }, 
            .size = 1});
          char * attrs = malloc(strlen(token) + 1);
          strcpy(attrs, token);

          // No support for nested groups

          finish = strtok_(&token, &buffer, 0, (Search_Until){
            .data = { "/g>" }, 
            .size = 1});
          char * body = malloc(strlen(token) + 1);
          strcpy(body, token);

          Group g = parse_group(body, attrs);

          print_group(g);

          PathParsed p[4] = {};

          int group_vertices_size = 0;
          int group_indices_w_size = 0;
          int group_indices_s_size = 0;

          for(int i=0; i<4; i++){
            p[i] = parse_path(g.children[i]);
            if(!strlen(p[i].name))
              continue;

            group_vertices_size += p[i].vertices.size * 2;
            group_indices_w_size += p[i].indices_wireframe.size;
            group_indices_s_size += p[i].indices_solid.size;

            fprintf(out, "const float %s%s[%d] = {\n", "vertices_", p[i].name,
              p[i].vertices.size * 2);
            fprintf(out, "\t");
            for(int j=0; j<p[i].vertices.size; j++)
              fprintf(out, "%.4ff, %.4ff, ", p[i].vertices.data[j].x, p[i].vertices.data[j].y);
            fprintf(out, "\n};\n");

            fprintf(out, "const unsigned char %s%s[%d] = {\n", "indices_w_", p[i].name,
              p[i].indices_wireframe.size);
            fprintf(out, "\t");
            for(int j=0; j<p[i].indices_wireframe.size; j++)
              fprintf(out, "%d, ", p[i].indices_wireframe.data[j]);
            fprintf(out, "\n};\n");

            fprintf(out, "const unsigned char %s%s[%d] = {\n", "indices_s_", p[i].name,
              p[i].indices_solid.size);
            fprintf(out, "\t");
            for(int j=0; j<p[i].indices_solid.size; j++)
              fprintf(out, "%d, ", p[i].indices_solid.data[j]);
            fprintf(out, "\n};\n");
          }
          
          fprintf(out, "const float %s%s[%d] = {\n", "vertices_", g.name,
              group_vertices_size);
          for(int i=0; i<4; i++){
            if(!strlen(p[i].name))
              continue;
            fprintf(out, "\t");
            for(int j=0; j<p[i].vertices.size; j++)
              fprintf(out, "%.4ff, %.4ff, ", p[i].vertices.data[j].x, p[i].vertices.data[j].y);
            fprintf(out, "\n");
          }
          fprintf(out, "};\n");

          fprintf(out, "const unsigned char %s%s[%d] = {\n", "indices_w_", g.name,
              group_indices_w_size);
          for(int i=0, k=0; i<4; i++){
            if(!strlen(p[i].name))
              continue;
            fprintf(out, "\t");
            for(int j=0; j<p[i].indices_wireframe.size; j++)
              fprintf(out, "%d, ", p[i].indices_wireframe.data[j] + k);
            k+=p[i].vertices.size;
            fprintf(out, "\n");
          }
          fprintf(out, "};\n");

          fprintf(out, "const unsigned char %s%s[%d] = {\n", "indices_s_", g.name,
              group_indices_s_size);
          for(int i=0, k=0; i<4; i++){
            if(!strlen(p[i].name))
              continue;
            fprintf(out, "\t");
            for(int j=0; j<p[i].indices_solid.size; j++)
              fprintf(out, "%d, ", p[i].indices_solid.data[j] + k);
            k+=p[i].vertices.size;
            fprintf(out, "\n");
          }
          fprintf(out, "};\n");

          free(body);
          free(attrs);
          fclose(out);
        }
    }
  } while (!finish);

  // free(...); will be done by OS 
}

int main() {
  const char *output_shaders_file = "build/resources.c";

  FILE *out_shaders = fopen(output_shaders_file, "w");
  if (!out_shaders) {
    perror("Failed to open output file");
    return EXIT_FAILURE;
  }
  fprintf(out_shaders, "// Auto-generated file for embedded shader sources\n\n");
  fclose(out_shaders);

  write_shader(output_shaders_file, "shader_ui_v", "src/resources/shader_ui.vert");
  write_shader(output_shaders_file, "shader_ui_f", "src/resources/shader_ui.frag");
  write_shader(output_shaders_file, "shader_game_v", "src/resources/shader_game.vert");
  write_shader(output_shaders_file, "shader_game_f", "src/resources/shader_game.frag");

  printf("Shader sources successfully embedded in %s\n", output_shaders_file);
    
  const char *output_svg_file = "build/shapes.c";

  FILE *out_svg = fopen(output_svg_file, "w");
  if (!out_svg) {
    perror("Failed to open output file");
    return EXIT_FAILURE;
  }
  fprintf(out_svg, "// Auto-generated file for embedded shapes sources\n\n");
  fclose(out_svg);

  write_svg(output_svg_file,  "src/resources/shapes.svg");
    
  printf("SVG sources successfully embedded in %s\n", output_svg_file);

  return 0;
}
