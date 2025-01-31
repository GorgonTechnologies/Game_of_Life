#version 330 core
in vec3 fragColor;
in float fragState;
in vec2 texCoord;
uniform sampler2D uGridTexture;
out vec4 FragColor;

void main() {
    if(fragState < 0.5){ 
      discard; 
    } else if(fragState < 1.0){ 
      FragColor = texture(uGridTexture, texCoord);
    } else { 
      FragColor = vec4(fragColor, 1.0);
    }
};
    