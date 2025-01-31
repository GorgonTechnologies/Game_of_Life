#define GL_GLEXT_PROTOTYPES
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

const char* vertexShaderSource = 
"#version 330 core\n"
"layout(location = 0) in vec2 aPos;\n"
"layout(location = 1) in vec2 iPos;\n"
"out vec3 fragColor;\n"

"void transform(float flip){\n"
"  mat4 t = mat4(\n"
"     1.0,     0.0,     0.0,    0.0,  \n" 
"     0.0,     flip,    0.0,    0.0,  \n" 
"     0.0,     0.0,     1.0,    0.0,  \n"
"     iPos.x,  iPos.y,  0.0,    1.0); \n"
"    gl_Position =  t * vec4(aPos, 0.0, 1.0);\n"
"}\n"

"void main() {\n"
"    transform( gl_InstanceID % 2 != 0 ? 1.0 : -1.0 ); \n"

"    if (gl_VertexID < 3) fragColor = vec3(1.0, 0.0, 0.0); \n"
"    else if(gl_VertexID < 6) fragColor = vec3(0.0, 1.0, 0.0);\n"
"    else fragColor = vec3(0.0, 0.0, 1.0);\n"
"}\n";

const char* fragmentShaderSource =
"#version 330 core\n"
"in vec3 fragColor;\n"
"out vec4 color;\n"
"void main() {\n"
"    color = vec4(fragColor, 1.0);\n"
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

GLuint createProgram(){
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    checkShaderCompilation(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    checkShaderCompilation(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkProgramLinking(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Instanced Rendering", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        fprintf(stderr, "Error creating OpenGL context: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1);

    float vertices[] = {
         0.0f,  0.2f,  
        -0.2f, -0.2f,  
         0.0f, -0.05f,

         0.0f,  0.2f,  
         0.0f, -0.05f,  
         0.2f, -0.2f, 

         0.0f, -0.05f,  
        -0.2f, -0.2f,  
         0.2f, -0.2f
    };

    unsigned int indices[] = {
        0, 1, 2,
        3, 4, 5,
        6, 7, 8
    };

    float instancePositions[] = {
        -0.45f,   0.21f,
        -0.225f,  0.21f,  
         0.0f,    0.21f,  
         0.225f,  0.21f,
         0.45f,   0.21f,  

        -0.45f,  -0.21f,
        -0.225f, -0.21f,  
         0.0f,   -0.21f,  
         0.225f, -0.21f,
         0.45f,  -0.21f 
    };

    GLuint VAO, VBO, EBO, positionVBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &positionVBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(instancePositions), instancePositions, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1); 

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    GLuint shaderProgram = createProgram();

    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = 0;
            }
        }

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawElementsInstanced(GL_TRIANGLES, 9, GL_UNSIGNED_INT, 0, 10);

        SDL_GL_SwapWindow(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &positionVBO);
    glDeleteProgram(shaderProgram);

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
