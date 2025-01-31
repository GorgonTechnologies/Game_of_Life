#define GL_GLEXT_PROTOTYPES
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>
#include <cglm/cglm.h>
#include <stdio.h>
#include <stddef.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

// Vertex shader for instanced rendering
const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec3 instanceColor;\n"
    "layout (location = 2) in mat4 instanceTransform;\n"
    "uniform mat4 projection;\n"
    "out vec3 fragColor;\n"
    "void main() {\n"
    "    gl_Position = projection * instanceTransform* vec4(aPos, 0.0, 1.0);\n"
    "    fragColor = instanceColor;\n"
    "}\n";

// Fragment shader (red squares)
const char* fragmentShaderSource = "#version 330 core\n"
    "in vec3 fragColor;\n"  
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = vec4(fragColor, 1.0);\n"
    "}\n";

// Square template (vertex positions)
float vertices[] = {
    1.0f,  1.0f,  // top-right
    1.0f, -1.0f,  // bottom-right
   -1.0f, -1.0f,  // bottom-left
   -1.0f,  1.0f   // top-left
};

int numInstances = 4;  // Number of squares to render

struct Instance{
    float color[3];
    mat4 transform;
};

struct Instance instanses[4] = {
    {.color = {1.0f, 0.0f, 0.0f}},
    {.color = {0.0f, 1.0f, 0.0f}},
    {.color = {0.0f, 0.0f, 1.0f}},
    {.color = {1.0f, 1.0f, 0.0f}}
};

unsigned int indices[] = { 0, 1, 3, 1, 2, 3 };

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

void APIENTRY openglDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    fprintf(stderr, "OpenGL Debug: %s\n", message);
}

int main() {
    // Initialize SDL
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

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(openglDebugCallback, NULL);

    // Compile shaders and create program
    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    // Set up vertex data
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    for (int i = 0; i < numInstances; i++) {
        glm_mat4_identity(instanses[i].transform);
        glm_translate(instanses[i].transform, (vec3){ (i - 1.5f) * 1.5f, 0.0f, 0.0f });  // Spread squares horizontally
        glm_scale(instanses[i].transform, (vec3){ 0.5f, 0.5f, 1.0f });  // Scale squares to half size
    }
    GLuint instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(instanses), instanses, GL_STATIC_DRAW);

    // Set up instanced attributes
    for (int i = 0; i < 4; i++) {  // Each column of the mat4                   
        glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(struct Instance), (void*)offsetof(struct Instance, transform) + (sizeof(vec4) * i));
        glEnableVertexAttribArray(2 + i);
        glVertexAttribDivisor(2 + i, 1);  // Tell OpenGL this is per-instance data
    
        /*
            Passing a mat4 (4x4 matrix) to a shader using glVertexAttribPointer involves a specific
            technique because OpenGL treats attributes as arrays of vectors. A mat4 requires 4 separate
            attribute locations, each for a column (or row, depending on your layout) of the matrix.

            Here’s a step-by-step explanation:

            1. Why mat4 Needs 4 Attribute Locations
                A mat4 is essentially a collection of 4 vec4 vectors.
                Each vec4 requires its own attribute location.
                If your mat4 is located at location = 2, OpenGL will use locations 2, 3, 4, and 5 for
                the 4 vec4 columns (or rows).
            
            2. Setting Up the Vertex Data
                Assume you have vertex data that includes a mat4:

                struct Vertex {
                    glm::vec3 position;  // Position (x, y, z)
                    glm::mat4 transform; // Transformation matrix
                };

                float vertices[] = {
                 // Position            Matrix (column-major order)
                    0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f,  // Column 0
                                        0.0f, 1.0f, 0.0f, 0.0f,  // Column 1
                                        0.0f, 0.0f, 1.0f, 0.0f,  // Column 2
                                        0.0f, 0.0f, 0.0f, 1.0f,  // Column 3
                    // Repeat for other vertices...
                };

            3. Using glVertexAttribPointer for mat4
                Binding the Buffer:
                First, bind your buffer as usual:

                GLuint VBO;
                glGenBuffers(1, &VBO);
                glBindBuffer(GL_ARRAY_BUFFER, VBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

                Setting Vertex Attribute Pointers:
                For a mat4, set up 4 separate attribute pointers, one for each column of the matrix.
                Assuming the matrix starts after the position data:

                // Position attribute
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
                glEnableVertexAttribArray(0);

                // Matrix attribute (4 vec4s)
                for (int i = 0; i < 4; i++) {
                    glVertexAttribPointer(
                        1 + i,               // Location: 1, 2, 3, 4
                        4,                   // Size: vec4 has 4 components
                        GL_FLOAT,            // Type: floats
                        GL_FALSE,            // Normalization: No
                        sizeof(Vertex),      // Stride: Size of one Vertex
                        (void*)(offsetof(Vertex, transform) + i * sizeof(glm::vec4))
                    );
                    glEnableVertexAttribArray(1 + i); // Enable each attribute
                }

                Key Details:
                - Location Range: If the matrix starts at location 1, its columns will occupy locations 1, 2, 3, and 4.
                - Size per Column: Each column is a vec4, so size = 4.
                - Stride: The total size of a Vertex struct (e.g., sizeof(Vertex)).
                - Pointer Offset: Each column is offset by the size of one vec4 (i * sizeof(glm::vec4)).
            
            4. Shader Code
                In the vertex shader:

                #version 330 core
                layout(location = 0) in vec3 aPos;       // Position
                layout(location = 1) in mat4 aTransform; // Matrix (4 vec4 attributes)
                void main() {
                    gl_Position = aTransform * vec4(aPos, 1.0);
                }

                OpenGL automatically groups the 4 vec4s into a mat4 when accessed in the shader.

            5. Matrix Storage Order
                By default, OpenGL expects matrices in column-major order. This means:

                Columns are stored consecutively in the buffer.
                If you're using libraries like GLM, no extra work is needed because GLM uses column-major order by default.
        */
    }

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(struct Instance), (void*)offsetof(struct Instance, color));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1); // Tell OpenGL to use one color per instance

    /*
        The function glVertexAttribDivisor is used in OpenGL for instanced rendering,
        which allows you to render multiple instances of a set of vertices with slight
        variations, such as position, color, or transformation matrices.

        **Signature
        void glVertexAttribDivisor(GLuint index, GLuint divisor);

        **Arguments
        1. index
            This specifies the index of the vertex attribute array to modify.
            This corresponds to the location of the attribute in your vertex shader,
            as specified by layout(location = index) or assigned automatically by OpenGL.
        2. divisor
            This determines the frequency at which the vertex attribute advances during instanced rendering:

            divisor = 0 (Default):
                The attribute is treated as per-vertex. It changes with every vertex in the draw call.
            divisor = 1:
                The attribute is treated as per-instance. It changes once per instance in the draw call.
            divisor = n (where n > 1):
                The attribute advances once every n instances. For example:
                A divisor of 2 means the attribute changes for every 2 instances.
                A divisor of 3 means the attribute changes for every 3 instances.

        **Use Case: Instanced Rendering
        Problem Without glVertexAttribDivisor
            In normal rendering, attributes like position, color, or matrices are fetched per vertex.
            This makes it difficult to reuse the same geometry for multiple instances of an object.

        Solution: Using glVertexAttribDivisor
            With glVertexAttribDivisor, you can specify attributes that update less frequently
            (e.g., once per instance) to provide instance-specific information, such as transformations
            or instance-specific colors.        
    */

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Projection matrix
    mat4 projection;
    glm_ortho(-4.0f, 4.0f, -3.0f, 3.0f, -1.0f, 1.0f, projection);

    GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    if (projectionLoc == -1) {
        fprintf(stderr, "Error: Unable to find uniform 'projection'\n");
    }   
    // Main loop
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

        // Send projection matrix to shader
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, (const GLfloat*)projection);

        glBindVertexArray(VAO);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, numInstances);

        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &instanceVBO);
    glDeleteProgram(shaderProgram);

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
