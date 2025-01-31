#version 330 core
    layout (location = 0) in vec2 aPos;   // vertex position
    layout (location = 1) in vec3 iColor; // color defined in instance
    layout (location = 2) in vec3 iPos;   // position shift and vertical flip defined in instance
    layout (location = 3) in float iState;// state (0/1) defined in instance
    uniform mat4 uProjection;             // projection matrix for viewport setup
    uniform vec3 uColor;                  // if need to override instance color 
                                          //   (example: for wireframe grid has to in one color)
    uniform float uState;                 // if need to override instance state 
                                          //   (example: for wireframe need be draw all instances -
                                          //   so state=on for all, for solid mode -
                                          //   need to draw only instances with state=on)
    uniform float uShape;
    out vec3 fragColor;
    out float fragState;
    
    out vec2 texCoord;

    mat4 flip = mat4(
      1.0, 0.0, 0.0, 0.0, 
      0.0, iPos.z, 0.0, 0.0, 
      0.0, 0.0, 1.0, 0.0,
      iPos.x, iPos.y, 0.0, 1.0);
    
    vec2 textureCoord[13] = vec2[](
      // Tetragon
      vec2(0.0f, 0.0f),
      vec2(1.0f, 0.0f),
      vec2(1.0f, 1.0f),
      vec2(0.0f, 1.0f),
      // Trigon
      vec2(0.0f, 0.0f),
      vec2(1.0f, 0.0f),
      vec2(0.5f, 0.8f),
      // Hexagon
      vec2(0.5f, 0.0f),
      vec2(1.0f, 0.25f),
      vec2(1.0f, 0.75f),
      vec2(0.5f, 1.0f),
      vec2(0.0f, 0.75f),
      vec2(0.0f, 0.25f)
    );

    void main() {
        gl_Position = uProjection * flip * vec4(aPos, 0.0, 1.0);
        fragColor = uColor.x > 0 ? uColor : iColor;
        fragState = uState > -1 ? uState : iState;
        if(uShape == 0.0)
          texCoord = textureCoord[gl_VertexID];
        else if(uShape == 1.0)
          texCoord = textureCoord[gl_VertexID + 4];
        else 
          texCoord = textureCoord[gl_VertexID + 7];
    };
