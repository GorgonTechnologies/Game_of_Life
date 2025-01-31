#version 330 core

in vec3 ourColor;
out vec4 FragColor;

uniform vec3 uColor;

void main(){
    if(uColor.r > 0)
        FragColor = vec4(uColor, 1.0);
    else
        FragColor = vec4(ourColor, 1.0);
}
