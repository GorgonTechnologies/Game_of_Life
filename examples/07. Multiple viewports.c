#define GL_GLEXT_PROTOTYPES
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>
#include <cglm/cglm.h>
#include <stdio.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

// Vertex shader source
const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 u_MVP;\n"
    "void main() {\n"
    "    gl_Position = u_MVP * vec4(aPos, 1.0);\n"
    "}\n";

// Fragment shader source (Red color for graphics)
const char* fragmentShaderSourceGraphics = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "}\n";

// Fragment shader source (Green color for UI)
const char* fragmentShaderSourceUI = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
    "}\n";

// Vertex data for a square
float vertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.5f,  0.5f, 0.0f,
    -0.5f,  0.5f, 0.0f
};
unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };

// Function to compile a shader
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "Error: Shader compilation failed\n%s\n", infoLog);
        return 0;
    }
    return shader;
}

// Function to create shader program
GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "Error: Program linking failed\n%s\n", infoLog);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

int main() {
    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
        return -1;
    }

    // Create SDL window
    SDL_Window* window = SDL_CreateWindow("Split Viewports", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        fprintf(stderr, "Error creating OpenGL context: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // Enable modern OpenGL features
    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1);

    // Create shader programs
    GLuint graphicsShader = createShaderProgram(vertexShaderSource, fragmentShaderSourceGraphics);
    GLuint uiShader = createShaderProgram(vertexShaderSource, fragmentShaderSourceUI);

    // Set up vertex data and buffers
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Main loop
    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = 0;
            }
        }

        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Left viewport (Graphics)
        glViewport(0, 0, SCREEN_WIDTH * 0.7, SCREEN_HEIGHT);
        glUseProgram(graphicsShader);

        float leftAspect = (float)(SCREEN_WIDTH * 0.7) / SCREEN_HEIGHT;
        mat4 projection, model, mvp;
        glm_ortho(-1.0f * leftAspect, 1.0f * leftAspect, -1.0f, 1.0f, -1.0f, 1.0f, projection);

        glm_mat4_identity(model);
        glm_mat4_mul(projection, model, mvp);

        GLuint mvpLoc = glGetUniformLocation(graphicsShader, "u_MVP");
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, (const GLfloat*)mvp);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Right viewport (UI)
        glViewport(SCREEN_WIDTH * 0.7, 0, SCREEN_WIDTH * 0.3, SCREEN_HEIGHT);
        glUseProgram(uiShader);

        float rightAspect = (float)(SCREEN_WIDTH * 0.3) / SCREEN_HEIGHT;
        glm_ortho(-1.0f * rightAspect, 1.0f * rightAspect, -1.0f, 1.0f, -1.0f, 1.0f, projection);

        // Scale factor for squares
        float scaleFactor = 0.3f / 0.7f;

        // Render first square (above center)
        glm_mat4_identity(model);
        glm_scale(model, (vec3){ scaleFactor, scaleFactor, 1.0f });
        glm_rotated(model, glm_rad(45), (vec3){ 0.0f, 0.0f, 1.0f });
        glm_translate(model, (vec3){ 0.0f, 0.6f, 0.0f });  // Move up
        glm_mat4_mul(projection, model, mvp);

        mvpLoc = glGetUniformLocation(uiShader, "u_MVP");
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, (const GLfloat*)mvp);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Render second square (below center)
        glm_mat4_identity(model);
        glm_scale(model, (vec3){ scaleFactor, scaleFactor, 1.0f });
        glm_translate(model, (vec3){ 0.0f, -0.6f, 0.0f });  // Move down
        glm_mat4_mul(projection, model, mvp);

        mvpLoc = glGetUniformLocation(uiShader, "u_MVP");
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, (const GLfloat*)mvp);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Swap buffers
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(graphicsShader);
    glDeleteProgram(uiShader);

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
