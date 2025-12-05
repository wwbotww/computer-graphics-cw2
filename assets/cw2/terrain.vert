#version 410 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vNormal;
out vec3 vWorldPos;
out vec2 vTexCoord;

void main()
{
	vec4 worldPos = uModel * vec4( aPosition, 1.0 );
	vWorldPos = worldPos.xyz;

	mat3 normalMatrix = mat3( transpose( inverse( uModel ) ) );
	vNormal = normalize( normalMatrix * aNormal );
	vTexCoord = aTexCoord;

	gl_Position = uProj * uView * worldPos;
}

