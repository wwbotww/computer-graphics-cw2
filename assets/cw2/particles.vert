#version 410 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aSizeAlpha; // x=size, y=alpha

uniform mat4 uView;
uniform mat4 uProj;
uniform float uViewportHeight;
uniform float uTanHalfFov;

out float vAlpha;

void main()
{
	vec4 viewPos = uView * vec4( aPosition, 1.0 );

	// 透视正确的像素尺寸： size_world * (H / (2*tan(fov/2))) / depth
	float dist = max( -viewPos.z, 0.001 );
	float pixelScale = (uViewportHeight * 0.5) / uTanHalfFov;
	gl_PointSize = aSizeAlpha.x * pixelScale / dist;

	vAlpha = aSizeAlpha.y;

	gl_Position = uProj * viewPos;
}


