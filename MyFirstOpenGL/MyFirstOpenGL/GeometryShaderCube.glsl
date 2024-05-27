#version 440 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 6) out;
uniform mat4 view;
uniform mat4 projection;

void main(){

	for(int i = 0; i < gl_in.length(); i++){
		gl_Position = projection * view * gl_in[i].gl_Position;
		EmitVertex();
	}

	EndPrimitive();
}