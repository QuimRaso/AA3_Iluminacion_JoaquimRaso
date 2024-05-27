#version 440 core

layout(location = 0) in vec3 posicion;
layout(location = 1) in vec2 uvsVertexShader;

out vec2 uvsGeometryShader;

void main() {

    uvsGeometryShader = uvsVertexShader;

    gl_Position = vec4(posicion, 1.0);

}