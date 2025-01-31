#define GL_GLEXT_PROTOTYPES
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

/*
1. Vertex Shader Execution:
  - All vertices are processed by the vertex shader regardless of their position.
  - The output of the vertex shader (gl_Position) is in clip space (homogeneous
    coordinates).
  - These coordinates undergo perspective division to convert them to normalized
    device coordinates (NDC).
2. Clipping:
  - Before reaching the rasterizer, OpenGL performs clipping against the canonical
    view volume:
    - In NDC, the x, y, and z coordinates must fall within the range [-1, 1] (after
      perspective division).
    - Vertices outside this range are clipped, and any primitives partially outside
      the volume are split into smaller primitives that fit within it.
3. Viewport and Scissor Test:
  - After clipping and viewport transformation, OpenGL applies the scissor test if
    enabled. Fragments outside the scissor rectangle are discarded.
  - If the vertex coordinates (after transformation) fall completely outside the
    scissor rectangle, the corresponding primitives won't produce any fragments.
4. Rasterizer and Fragment Shader:
  - Only primitives or portions of primitives that are inside the viewport/scissor
    bounds produce fragments.
  - If a vertex is outside the bounds but part of a primitive (e.g., a triangle), the
    primitive may still produce fragments if other vertices bring part of the primitive
    into view.
  - Fragments outside the bounds won't be generated, so the fragment shader won't run
    for them.

    Setting `gl_Position` in Vertex shader outside of clipping bounds is equivalent of
    `discard` command in Fragmant shader 
*/

const char* vertexShaderSource = 
"#version 330 core\n"
"layout(location = 0) in vec2 position;\n"
"layout(location = 1) in vec3 instanceColor;\n"
"layout(location = 2) in vec2 instancePosition;\n"
"out vec3 fragColor;\n"
"void main() {\n"
"    gl_Position = vec4( position + instancePosition, 0.0, 1.0);\n"
"    if (gl_InstanceID == 0 && gl_VertexID > 2 || \n"
"        gl_InstanceID == 1 && gl_VertexID < 3) {\n"
"       gl_Position = vec4(0.0,0.0,0.0,1.0);\n"
"    }\n"
"    fragColor = instanceColor;\n"
"}\n";

const char* fragmentShaderSource =
"#version 330 core\n"
"in vec3 fragColor;\n"
"out vec4 color;\n"
"void main() {\n"
"    color = vec4(fragColor, 1.0);\n"
"}";

/*
1. Vertex Shader
  - gl_VertexID:
    Description: The zero-based index of the vertex being processed, as determined by the draw call.
    Use Case: Useful for indexing into VBOs or performing calculations based on vertex order.
  - gl_InstanceID:
    Description: The zero-based index of the current instance being processed during instanced rendering.
    Use Case: Differentiating instances in instanced rendering.
  - gl_Position:
    Description: The output position of the vertex in clip space. You must set this in the vertex shader for proper rendering.
    Use Case: Determines the position of vertices on the screen.
  - gl_PointSize:
    Description: Specifies the size of a point primitive in pixels.
    Use Case: Adjusting point size when rendering points (GL_POINTS).
2. Fragment Shader
  - gl_FragCoord:
    Description: The window-relative coordinate of the fragment being processed.
    Use Case: Implementing effects like screen-space shading, depth-based effects, or pixel-specific calculations.
  - gl_FrontFacing:
    Description: A boolean indicating whether the current fragment belongs to a front-facing primitive.
    Use Case: Differentiating front-facing and back-facing fragments for effects like culling or two-sided shading.
  - gl_FragDepth:
    Description: Allows you to explicitly set the depth value for the fragment.
    Use Case: Overriding depth for custom depth-testing behavior.
  - gl_SampleID:
    Description: The index of the current sample being processed in a multisample context.
    Use Case: Multisample anti-aliasing (MSAA) effects.
  - gl_SamplePosition:
    Description: The position of the current sample within the pixel, given in normalized coordinates [0,1].
    Use Case: Sub-pixel precision effects in multisampling.
  - gl_HelperInvocation:
    Description: A boolean indicating if the current invocation is a helper invocation (used for derivative calculations).
    Use Case: Debugging and ensuring correct behavior of derivatives.
  - gl_Layer (fragment shaders):
    Used for selecting layers in layered rendering (e.g., in framebuffer objects with multiple layers).
3. Compute Shader
  - gl_GlobalInvocationID:
    Description: The global ID of the current invocation.
    Use Case: Identifying work items in a global space.
  - gl_LocalInvocationID:
    Description: The local ID of the current invocation within a workgroup.
    Use Case: Managing shared memory or local computations.
  - gl_WorkGroupID:
    Description: The ID of the current workgroup.
    Use Case: Organizing workloads into groups.
  - gl_NumWorkGroups:
    Description: The total number of workgroups.
    Use Case: Managing global dispatch.
  - gl_WorkGroupSize:
    Description: The size of a single workgroup.
    Use Case: Controlling workgroup dimensions.
*/

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
        // Triangle
        -0.2f, -0.8f,  
         0.2f, -0.8f,  
         0.0f, -0.4f, 
        // Square
        -0.2f,  0.4f,  
         0.2f,  0.4f,  
         0.2f,  0.8f,
        -0.2f,  0.8f
    };

    unsigned int indices[] = {
        // Triangle
        0, 1, 2,
        // Square
        3, 4, 6,
        4, 5, 6
    };

    unsigned char instanceColors[] = {
        255, 0, 0,  
        0, 255, 0,  
        0, 0, 255   
    };

    float instancePositions[] = {
        -0.7f,  0.0f,  
         0.0f,  0.0f,  
         0.7f,  0.0f   
    };

    GLuint VAO, VBO, EBO, colorVBO, positionVBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &colorVBO);
    glGenBuffers(1, &positionVBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(instanceColors), instanceColors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, 3 * sizeof(unsigned char), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1); 

    glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(instancePositions), instancePositions, GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1); 

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
        glDrawElementsInstanced(GL_TRIANGLES, 9, GL_UNSIGNED_INT, 0, 3);

        SDL_GL_SwapWindow(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &colorVBO);
    glDeleteBuffers(1, &positionVBO);
    glDeleteProgram(shaderProgram);

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
