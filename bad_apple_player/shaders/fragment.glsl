#version 450 core

layout (location = 0) out vec4 fColor;

uniform vec4 vColor;

void main() {
    fColor = vColor;
}