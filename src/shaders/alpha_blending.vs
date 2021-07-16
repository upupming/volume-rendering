#version 330 core

layout(location=0)in vec4 a_position;

uniform mat4 mvpMatrix;

void main(){
    gl_Position=mvpMatrix*a_position;
}
