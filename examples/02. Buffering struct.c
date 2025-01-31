#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

// Define the Vertex structure
struct Vertex {
    float x, y;            
    unsigned char r, g, b; 
};

// Define the Shape structure
struct Shape {
    struct Vertex* vertices;
    char* indices;
    char vertices_size;
    char indices_size;
};

// Function to initialize shapes
void initializeShapes(struct Shape shapes[4]) {
    // Define the first triangle
    static struct Vertex triangleVertices[] = {
        { 0.0f,  0.5f, 255,   0,   0 }, // Top (Red)
        { 0.5f, -0.5f,   0, 255,   0 }, // Bottom-right (Green)
        {-0.5f, -0.5f,   0,   0, 255 }  // Bottom-left (Blue)
    };
    static char triangleIndices[] = { 0, 1, 2 };

    // Define the second rectangle
    static struct Vertex rectangleVertices[] = {
        { -0.5f,  0.5f, 255, 255,   0 }, // Top-left (Yellow)
        {  0.5f,  0.5f, 255,   0, 255 }, // Top-right (Magenta)
        {  0.5f, -0.5f,   0, 255, 255 }, // Bottom-right (Cyan)
        { -0.5f, -0.5f, 255, 255, 255 }  // Bottom-left (White)
    };
    static char rectangleIndices[] = { 0, 1, 2, 0, 2, 3 };

    // Define shapes
    shapes[0].vertices = triangleVertices;
    shapes[0].indices = triangleIndices;
    shapes[0].vertices_size = sizeof(triangleVertices);
    shapes[0].indices_size = sizeof(triangleIndices);

    shapes[1].vertices = rectangleVertices;
    shapes[1].indices = rectangleIndices;
    shapes[1].vertices_size = sizeof(rectangleVertices);
    shapes[1].indices_size = sizeof(rectangleIndices);

    // Repeat for the third and fourth shapes (for simplicity, using the same shapes)
    shapes[2] = shapes[0];
    shapes[2].indices = triangleIndices;
    shapes[2].vertices_size = sizeof(triangleVertices);
    shapes[2].indices_size = sizeof(triangleIndices);

    shapes[3] = shapes[1];
    shapes[3].indices = rectangleIndices;
    shapes[3].vertices_size = sizeof(rectangleVertices);
    shapes[3].indices_size = sizeof(rectangleIndices);
}

  // Helper function to compile shaders
  GLuint compileShader(GLenum type, const char* source) {
      GLuint shader = glCreateShader(type);
      glShaderSource(shader, 1, &source, NULL);
      glCompileShader(shader);
      return shader;
  };
  
// Main program
int main() {
    // Initialize GLFW
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
        return -1;
    }

    // Create SDL window
    SDL_Window* window = SDL_CreateWindow("Instanced Rendering", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        fprintf(stderr, "Error creating OpenGL context: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1);

    // Initialize shapes
    struct Shape shapes[4] = {};
    initializeShapes(shapes);

    // Calculate buffer sizes
    size_t vertexDataSize = 0, indexDataSize = 0;
    for (int i = 0; i < 4; i++) {
        vertexDataSize += shapes[i].vertices_size;
        indexDataSize += shapes[i].indices_size;
    }

    // Create a universal buffer
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, vertexDataSize + indexDataSize, NULL, GL_STATIC_DRAW);

    // Upload vertex and index data
    size_t vertexOffset = 0, indexOffset = vertexDataSize;
    for (int i = 0; i < 3; i++) {
        glBufferSubData(GL_ARRAY_BUFFER, vertexOffset, shapes[i].vertices_size, shapes[i].vertices);
        vertexOffset += shapes[i].vertices_size;

        glBufferSubData(GL_ARRAY_BUFFER, indexOffset, shapes[i].indices_size, shapes[i].indices);
        indexOffset += shapes[i].indices_size;
    }

    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSize, NULL, GL_STATIC_DRAW);

    indexOffset = 0;
    for (int i = 0; i < 4; i++) {
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indexOffset, shapes[i].indices_size, shapes[i].indices);
        indexOffset += shapes[i].indices_size;
    }

    // Set up VAO
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); 

    // Configure vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), (void*)offsetof(struct Vertex, x));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct Vertex), (void*)offsetof(struct Vertex, r));
    glEnableVertexAttribArray(1);

    

    // Simple shader program
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "layout (location = 1) in vec3 aColor;\n"
        "layout (location = 2) in vec2 aOffset;\n"
        "out vec3 vertexColor;\n"
        "void main() {\n"
        "    gl_Position = vec4(aPos + aOffset, 0.0, 1.0);\n"
        "    vertexColor = aColor;\n"
        "}";
                                
    const char* fragmentShaderSource = "#version 330 core\n"
                                       "in vec3 vertexColor;\n"
                                       "out vec4 FragColor;\n"
                                       "void main() {\n"
                                       "    FragColor = vec4(vertexColor, 1.0);\n"
                                       "}";

    // Compile shaders and link program
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Main render loop
    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = 0;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        size_t currentIndexOffset = 0;
        for (int i = 0; i < 4; i++) {
            glDrawElements(GL_TRIANGLES, shapes[i].indices_size, GL_UNSIGNED_BYTE, (void*)currentIndexOffset);
            currentIndexOffset += shapes[i].indices_size;
        }

        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    glDeleteBuffers(1, &buffer);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderProgram);

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
