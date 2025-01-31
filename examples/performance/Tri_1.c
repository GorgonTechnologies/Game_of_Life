#define GL_GLEXT_PROTOTYPES
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <cglm/cglm.h>

//gcc-13 ./examples/performance/Tri_1.c -g -o main  -lGL -lcglm -lSDL3 -lm

// Instanced drawing - Vertex + Fragment shaders.
// Two draw calls - for wireframes and for solid
// EBO reallocation for each drawcall
// 3 vertices / 6 wireframe indices / 3 solid indices
// 3 lines + 1 triangle
// Elapsed time 0.905 ms

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 960

// #define GL_SDL_DEBUG

const char* vertexShaderSource = 
"#version 330 core\n"
"layout(location = 0) in vec2 aPos;\n"
"layout(location = 1) in vec3 iPos;\n"
"uniform mat4 uProjection;\n" 
"uniform float uState;\n" 
"void transform(float flip){\n"
"  float dx = (uState + iPos.z) > 0.0 ? iPos.x : -1000.0;\n"
"  float dy = iPos.y; \n"
"  mat4 t = mat4(\n"
"     1.0,    0.0,    0.0,    0.0,  \n" 
"     0.0,    flip,   0.0,    0.0,  \n" 
"     0.0,    0.0,    1.0,    0.0,  \n"
"     dx,     dy,     0.0,    1.0); \n"
"  gl_Position =  uProjection * t * vec4(aPos, 0.0, 1.0);\n"
"}\n"
"void main() { "
"    transform( gl_InstanceID % 2 != 0 ? 1.0 : -1.0 ); \n"
"} ";

const char* fragmentShaderSource =
"#version 330 core\n"
"out vec4 fragColor;\n"
"void main() {\n"
"    fragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
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

    float size = 5.0;
    float vertices[] = {
    //  x       y
        0.0f,       size*0.5,
       -size*0.5,  -size*0.5,
        size*0.5,  -size*0.5,
    };
    unsigned int indices_s[] = { 0, 1, 2 };
    unsigned int indices_w[] = { 0, 1, 1, 2, 2, 0 };

    int instanceSize = 400000;
    float* instanceState = calloc(instanceSize, sizeof(float));
    int n = 0;
    float s = 0;
    for(int i = 0, x = size, y = size; i<instanceSize ; ){
        if((x + size) < SCREEN_WIDTH){
            instanceState[i]   = x;
            instanceState[i+1] = y;
            instanceState[i+2] = s;
            x += size * 0.5;
            i+=3;
            n++;
            s = s > 0 ? 0.0 : 1.0;
        } else {
            if((y + size * 2) < SCREEN_HEIGHT){
                instanceState[i]   = x;
                instanceState[i+1] = y;
                instanceState[i+2] = 0.0;
                i+=3;
                n++;
                x = size;
                y += size;
                s = s > 0 ? 0.0 : 1.0; 
            } else {
                printf("%d, %d, %d\n", i, x, y);
                break;
            }
        }
    };

    GLuint VAO, VBO, EBO, stateVBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &stateVBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, stateVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * instanceSize, instanceState, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1); 

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    mat4 uProjection;
    glm_ortho(0.0f, SCREEN_WIDTH, 0.0f, SCREEN_HEIGHT, 0.0f, 1.0f, uProjection);

    GLuint shaderProgram = createProgram();
    GLuint uStateLoc = glGetUniformLocation(shaderProgram, "uState");
    GLuint uProjectionLoc = glGetUniformLocation(shaderProgram, "uProjection");

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    printf("%d\n", n);

    GLuint query;
    glGenQueries(1, &query);

    int running = 1;
    do {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = 0;
            }
        }
        #ifdef GL_SDL_DEBUG
        Uint32 frameStart = SDL_GetTicks();
        glBeginQuery(GL_TIME_ELAPSED, query);
        #endif

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices_w), indices_w, GL_STATIC_DRAW);
        glUniform1f(uStateLoc, 1.0);
        glUniformMatrix4fv(uProjectionLoc, 1, GL_FALSE, (const GLfloat*)uProjection);

        glDrawElementsInstanced(GL_LINES, 6, GL_UNSIGNED_INT, 0, n);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices_s), indices_s, GL_STATIC_DRAW);
        glUniform1f(uStateLoc, 0.0);

        glDrawElementsInstanced(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0, n);
        
        #ifdef GL_SDL_DEBUG
        glEndQuery(GL_TIME_ELAPSED);
        Uint32 frameEnd = SDL_GetTicks();
        float frameTime = (frameEnd - frameStart) / 1000.0f; // Seconds per frame
        float fps = 1.0f / frameTime;
        printf("FPS: %f\n", fps);
        GLuint elapsedTime;
        glGetQueryObjectuiv(query, GL_QUERY_RESULT, &elapsedTime);
        printf("Elapsed time: %f ms\n", elapsedTime / 1e6f);
        #endif
        SDL_GL_SwapWindow(window);
    } while (running);

    glDeleteQueries(1, &query);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &stateVBO);
    glDeleteProgram(shaderProgram);

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
