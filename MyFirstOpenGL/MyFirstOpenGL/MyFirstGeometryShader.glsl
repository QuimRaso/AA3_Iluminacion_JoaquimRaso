#version 440 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec2 uvsGeometryShader[];
out vec2 uvsFragmentShader;

uniform mat4 translationMatrix;
uniform mat4 rotationMatrix;
uniform mat4 scaleMatrix;
uniform mat4 view;
uniform mat4 projection;

void main(){

    mat4 model = translationMatrix * rotationMatrix * scaleMatrix;

	for(int i = 0; i < gl_in.length(); i++){
		gl_Position = projection * view * model * gl_in[i].gl_Position;
		uvsFragmentShader = uvsGeometryShader[i];
		EmitVertex();
	}

	EndPrimitive();
}