#version 410 core

layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec4 aColor;

uniform mat4 uProj;

out vec2 vUV;
out vec4 vColor;

void main()
{
	vUV = aUV;
	vColor = aColor;
	gl_Position = uProj * vec4( aPosition, 0.0, 1.0 );
}


