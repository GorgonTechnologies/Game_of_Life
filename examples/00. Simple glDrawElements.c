#define GL_GLEXT_PROTOTYPES
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>
#include <cglm/cglm.h>
#include <stdio.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#include <stdio.h>
#include <stdlib.h>

// Vertex shader source code
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec2 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"out vec3 ourColor;\n"
"void main()\n"
"{\n"
"    gl_Position = vec4(aPos, 0.0, 1.0);\n"
"    ourColor = aColor;\n"
"}\0";

// Fragment shader source code
const char* fragmentShaderSource = "#version 330 core\n"
"in vec3 ourColor;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"    FragColor = vec4(ourColor, 1.0);\n"
"}\0";

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
    SDL_Window* window = SDL_CreateWindow("Instanced Rendering", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        fprintf(stderr, "Error creating OpenGL context: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1);

    // Compile shaders and create program
    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    // Define vertex data (positions + colors)
    float vertices[] = {
        // Positions        // Colors
        -0.8f,  0.2f,      1.0f, 0.0f, 0.0f, // Square 1
        -0.8f, -0.2f,      1.0f, 0.0f, 0.0f,
        -0.4f, -0.2f,      1.0f, 0.0f, 0.0f,
        -0.4f,  0.2f,      1.0f, 0.0f, 0.0f,

        -0.2f,  0.2f,      0.0f, 1.0f, 0.0f, // Square 2
        -0.2f, -0.2f,      0.0f, 1.0f, 0.0f,
         0.2f, -0.2f,      0.0f, 1.0f, 0.0f,
         0.2f,  0.2f,      0.0f, 1.0f, 0.0f,

         0.4f,  0.2f,      0.0f, 0.0f, 1.0f, // Square 3
         0.4f, -0.2f,      0.0f, 0.0f, 1.0f,
         0.8f, -0.2f,      0.0f, 0.0f, 1.0f,
         0.8f,  0.2f,      0.0f, 0.0f, 1.0f,
    };

    // Define indices
    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,    // Square 1
        4, 5, 6, 6, 7, 4,    // Square 2
        8, 9, 10, 10, 11, 8  // Square 3
    };

    // Create VAO, VBO, and EBO
    unsigned int VAO, VBO, EBO;

    /*
        In modern OpenGL, Vertex Arrays, Buffers, and Vertex Attribute Pointers are used to manage
        and draw vertex data efficiently. Here's an explanation of each:

        1. Vertex Array Objects (VAOs)
            A Vertex Array Object (VAO) encapsulates the state of vertex attribute configurations.
            It doesn't store the vertex data itself but tracks the following:
            - Which buffers are bound to which target.
            - How the data in those buffers are interpreted (via vertex attribute pointers).
            Key Points:
            - VAOs allow you to switch between different vertex configurations easily.
            - Each VAO can be thought of as a "recipe" for how to use vertex data during rendering.
            Typical VAO Workflow:
            1. Generate a VAO with glGenVertexArrays.
            2. Bind the VAO with glBindVertexArray.
            3. Configure vertex attributes (using vertex attribute pointers).
            4. Bind buffers (e.g., VBOs, EBOs) and specify how data will be used.
            5. Unbind the VAO (optional) when configuration is complete.

        2. Buffers
            Buffers in OpenGL store raw data, like vertex positions, colors, normals, or indices.

            Common Types:
            - Vertex Buffer Objects (VBOs): Store vertex attribute data (positions, colors, etc.).
            - Element Buffer Objects (EBOs): Store index data for indexed drawing.
            Key Functions:
            - glGenBuffers: Generates buffer IDs.
            - glBindBuffer: Binds a buffer to a specific target (e.g., GL_ARRAY_BUFFER for VBOs
                or GL_ELEMENT_ARRAY_BUFFER for EBOs).
            - glBufferData: Allocates and optionally initializes buffer data.
            - glBufferSubData: Updates part of the buffer data.
        
        3. Vertex Attribute Pointers
            A Vertex Attribute Pointer defines how vertex data in a VBO is laid out and how OpenGL
            should interpret it. Each attribute (e.g., position, color, normal) has its own pointer.

            Key Functions:
            1. glVertexAttribPointer: Specifies the format of the vertex data.
                - Which attribute (location index) the data corresponds to.
                - How many components the attribute has (e.g., 2 for 2D position, 3 for RGB color).
                - The data type of the components.
                - Whether to normalize the data.
                - The stride (distance between consecutive vertices).
                - The offset (start of the attribute in the vertex data).
            2. glEnableVertexAttribArray: Enables the specified attribute location.

        Analogy
            VBO - Stores the raw vertex data (e.g., positions, colors).
            EBO - Tells OpenGL the order of drawing vertices.
            VAO - Keeps track of which data to use, how to interpret it, and how to draw it.
            Vertex Attribute Pointers - Specify the format and layout of the vetices in the VBO.

        Workflow Example: Putting It All Together
            1. Generate VAO and Buffers:

                GLuint VAO, VBO, EBO;
                glGenVertexArrays(1, &VAO);
                glGenBuffers(1, &VBO);
                glGenBuffers(1, &EBO);
            
            2. Bind and Configure VAO:

                glBindVertexArray(VAO);

                // Bind and upload vertex data to VBO
                glBindBuffer(GL_ARRAY_BUFFER, VBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

                // Bind and upload index data to EBO
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

                // Set vertex attribute pointers
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); // Position
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); // Texture coordinates
                glEnableVertexAttribArray(1);

                // Unbind VAO (optional for safety)
                glBindVertexArray(0);
            
            3. Render Loop:

                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    */

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Bind VAO
    glBindVertexArray(VAO);

    // Bind and populate VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Bind and populate EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Define vertex attributes
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    /*
        The function glVertexAttribPointer specifies how OpenGL should interpret a specific vertex
        attribute from the vertex data stored in a buffer (usually a VBO).
        Let’s break down the arguments:

        glVertexAttribPointer(
            GLuint index,
            GLint size,
            GLenum type,
            GLboolean normalized,
            GLsizei stride,
            const void* pointer
        );

        Example:
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        Breaking Down the Arguments
        1. GLuint index
            Specifies the attribute location in the shader program.
            In this example: 1 corresponds to the second attribute (the first is at location 0).
            Shader Link: This must match the layout qualifier or the location in the shader:

            layout(location = 1) in vec2 aTexCoord; // Example in vertex shader
        2. GLint size
            Specifies the number of components in this attribute.
            Example: 2 for a 2D texture coordinate (like (u, v)), 3 for a 3D position, etc.
            In this example: 2 indicates this attribute is a vec2 (e.g., (u, v)).
        3, GLenum type
            Specifies the data type of each component in the attribute.
            Examples:
            - GL_FLOAT: Floating-point numbers (float in C).
            - GL_UNSIGNED_BYTE: Unsigned 8-bit integers, etc.
            In this example: GL_FLOAT means the texture coordinates are stored as floats.
        4. GLboolean normalized
            Specifies whether the data should be normalized when passed to the shader.
            - If GL_TRUE: Values are normalized to the range [0, 1] (for unsigned types) or [-1, 1] (for signed types).
            - If GL_FALSE: Values are passed directly as is.
            In this example: GL_FALSE because the texture coordinates (e.g., (0.0, 1.0)) are already in the correct range.
        5. GLsizei stride
            Specifies the byte offset between consecutive vertex attributes.
            How to calculate:
                If all attributes for a single vertex are stored in a single structure, stride is the size of that structure.
            Example: If a vertex has 3 floats for position (x, y, z) and 2 floats for texture coordinates (u, v), the stride is:

            5 * sizeof(float) // (3 + 2) floats per vertex - so it is Total size of one vertex.

            In this example: 5 * sizeof(float) means the stride is 20 bytes (3 floats for position + 2 floats
            for texture coordinates).
        6. const void* pointer
            Specifies the byte offset to the start of this attribute in the vertex structure.
            How to calculate:
                It’s the offset to the data in the structure.
            For the texture coordinates (u, v), if the first 3 floats (x, y, z) represent the position, then:

                pointer = (void*)(3 * sizeof(float))

            This tells OpenGL to start reading after the first 12 bytes (3 floats).
            In this example: (void*)(3 * sizeof(float)) means the texture coordinates are located after the position data.
    */

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind VAO (optional)
    glBindVertexArray(0);

    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = 0;
            }
        }
        // Clear screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Use shader program and bind VAO
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        // Draw elements
        glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);

        // Swap buffers and poll events
       SDL_GL_SwapWindow(window);
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
