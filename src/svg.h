#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>

#ifndef SVG_H
#define SVG_H

 /*
    I only care about
    - groups
    - paths
    - defs/use -> templating

    Here would be quite primitive impl
    with support for not-nested groups and paths

    More like an exercise
  */

typedef struct{
  char name[16];
  char value[1024];
} Path;

typedef struct{
  unsigned char data[32];
  char size;
} u8_array;

typedef struct{
  float x;
  float y;
} Vertex;

typedef struct{
  Vertex data[8];
  char size;
} v_array;

typedef struct{
  char name[16];
  v_array vertices;
  u8_array indices_wireframe;
  u8_array indices_solid;
} PathParsed;

typedef struct{
  char name[16];
  Path children[4];
} Group;

void print_group(Group g){
  printf("\n");
  printf("%s:\n", g.name);
  for (int i = 0; i < 4; i++)
    if(strlen(g.children[i].name))
      printf("\t%s: \t%s\n", g.children[i].name, g.children[i].value);
    else 
      continue;
  printf("\n");
}

typedef enum{
  SEARCH,
  NAME, 
  VALUE
} State;

typedef struct{
  char data[4][4];
  int size;
} Search_Until;

int strtok_(char** token, char** buffer, char skip_until, Search_Until s){
  // for just reading there is no point copying data
  // just move pointers
    
  if(skip_until != 0)
    for(int i=0;;i++){
      if((*buffer)[i]==skip_until){
        *buffer = &((*buffer)[i+1]);
        break;
      }
      if((*buffer)[i]=='\0')
        return 1; 
    }

  for(int i=0; ;i++)
    for(int j=0; j < s.size; j++){
      if((*buffer)[i] == s.data[j][0]){
        int success = 1;
        
        for(int k=0; k<strlen(s.data[j]); k++){
          if((*buffer)[i+k]!= s.data[j][k])
            success = 0;
        }
        if(success){
          *token = *buffer;
          (*token)[i] = '\0';

          if((*buffer)[i + strlen(s.data[j]) + 1]!='\0'){
            *buffer = &((*buffer)[i+1]);
            return 0;
          } else
            return 1;
        }
      } else if((*buffer)[i]=='\0'){
        *token = *buffer;
        (*token)[i] = '\0';
        return 1;
      }
    }
  return 1;    
}

void parse_attribute(Group* group, Path* path, char **attrs){
  char * name = calloc(16, sizeof(char));
  char * value = calloc(1024, sizeof(char));
  
  #define ATTRIBUTE_ERROR do { \
    printf("Incorrect format");\
    free(name);                \
    free(value);               \
    return;                    \
  } while (0)

  #define SET_ATTRIBUTE do { \
    if(!strcmp(name, "id") || !strcmp(name, "d")){ \
      if(path == NULL){                            \
        for(int i = 0; i < 16; i++)                \
          group->name[i] = tolower(value[i]);      \
      } else {                                     \
        if(!strcmp(name, "id"))                    \
          for(int i = 0; i < 16; i++)              \
            path->name[i] = tolower(value[i]);    \
        else                                       \
          strcpy(path->value, value);              \
      }                                            \
    }                                              \
  } while (0)

  State state = SEARCH;
  for(int i=0; ; ){
    switch ((char)(*attrs)[i]){
      case '\"':{
        switch (state){
          case SEARCH:{ ATTRIBUTE_ERROR; }
          case NAME:{ 
            *attrs = &((*attrs)[i+1]);
            i=0;
            state = VALUE;
            break;
          }
          case VALUE:{
            value[i] = '\0';
            SET_ATTRIBUTE;
            *attrs = &((*attrs)[i+1]);
            i=0;
            state = SEARCH;
          }
        }
        break;
      }
      case '\n':
      case ' ':{
        switch (state){
          case SEARCH:{
            *attrs = &((*attrs)[i+1]);
            i=0;
            break;
          }
          case NAME:{ ATTRIBUTE_ERROR; }
          case VALUE:{
            value[i] = (*attrs)[i];
            i++;
            break;
          }
        }
        break;
      }
      case '=':{
        switch (state){
          case VALUE:
          case SEARCH:{ ATTRIBUTE_ERROR; }
          case NAME:{
            name[i] = '\0';
            *attrs = &((*attrs)[i+1]);
            i=0;
            break;
          } 
        }
        break;
      }
      case '\0':
      case '>':{
        switch (state){
          case SEARCH:{ return; }
          case NAME:{ ATTRIBUTE_ERROR; }
          case VALUE:{
            SET_ATTRIBUTE;
            return;
          }
        }
      }
      default:{
        switch (state){
          case SEARCH:{ state = NAME; }
          case NAME:{ name[i] = (*attrs)[i]; break; }
          case VALUE:{ value[i] = (*attrs)[i]; }
        }
        i++;
      }
    }
  }
  SET_ATTRIBUTE;
}

Group parse_group(char *paths, char *group_attrs){
  Group group = {};
  parse_attribute(&group, NULL, &group_attrs);

  char* token = NULL;
  for(int i=0; i<4; i++){
    strtok_(&token, &paths, '<', (Search_Until){
      .data = { "\n", " ", ">" }, 
      .size = 3});
    if(!strcmp("path", token)){
      printf("Processing path");
      strtok_(&token, &paths, 0, (Search_Until){
        .data = { ">" }, 
        .size = 1});
      char * attrs = malloc(strlen(token) + 1);
      strcpy(attrs, token);
      Path path = {};
      parse_attribute(NULL, &path, &attrs);
      group.children[i] = path;
    }
  }
  return (Group)group;
}

PathParsed parse_path(Path p){
  PathParsed pp = (PathParsed){};
  //memcpy(pp.name, p.name, 16);
  strcpy(pp.name, p.name);

  char mode = '\0';
  float queue[2] = {};  // we need max two nums deep queue
  char buffer[8] = {}; // buffer to collect chars before transform to float

  #define CLEAR_BUFFER  do {  \
    for(int ib=0; ib<8; ib++) \
      buffer[ib] = '\0';      \
    /* memset(&buffer, 0, 8 * sizeof(void *)); */ \
    } while (0);

  for(int i=0, b=0, q=0, v=0; i<strlen(p.value); i++){
    switch (p.value[i]){
    case 'L':
    case 'M':
    case 'V':
    case 'H':{ mode = p.value[i]; break; }
    case 'Z':{
      pp.vertices.size = v;
      break;
    }
    case ',':{
      b = 0;
      queue[q] = strtof(buffer, NULL);
      CLEAR_BUFFER;
      q++;
      break;
    }
    case ' ':{
      if(b){ // Could be space after comma
        b = 0;
        queue[q] = strtof(buffer, NULL);
        CLEAR_BUFFER;
        q++;
      }
      if(!q){
        // if no numbers
      } else{ 
        switch(mode){
          case 'L':{
            if(q==2){
              pp.vertices.data[v] = (Vertex){queue[0], queue[1]};
              v++;
              q = 0;
              break;
            }
          }
          case 'H':{
            if(q==1){
              pp.vertices.data[v] =
                (Vertex){queue[0], pp.vertices.data[v-1].y};
              v++;
              q = 0;
              break;
            }
          }
          case 'V':{
            if(q==1){
              pp.vertices.data[v] =
                (Vertex){pp.vertices.data[v-1].x, queue[0]};
              v++;
              q = 0;
              break;
            }
          }
          case 'M':{
            if(q==2){
              pp.vertices.data[v++] = (Vertex){queue[0], queue[1]};
              q = 0;
              break;
            }
          }
        }
      }
      break;
    }
    default:
      buffer[b] = p.value[i];
      b++;
      break;
    }
  }
  
  for(int i=1, j=0; i<pp.vertices.size; i++){
    pp.indices_wireframe.data[j++] = i-1;
    pp.indices_wireframe.data[j++] = i;
    if(i == pp.vertices.size - 1){
      pp.indices_wireframe.data[j++] = i;
      pp.indices_wireframe.data[j++] = 0;
      pp.indices_wireframe.size = j;
    }
  }

  for(int i=2, j=0; i<pp.vertices.size; i++){
    pp.indices_solid.data[j++] = i-2;
    pp.indices_solid.data[j++] = i-1;
    pp.indices_solid.data[j++] = i;
    if(i == pp.vertices.size - 1){
      pp.indices_solid.size = j;
    }
  }

  return pp;
}

#endif