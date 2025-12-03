#version 410 core

in vec3 vNormal;
in vec3 vWorldPos;

uniform vec3 uLightDir;
uniform vec3 uAmbientColor;
uniform vec3 uDiffuseColor;

out vec4 FragColor;

void main()
{
	vec3 normal = normalize( vNormal );
	vec3 lightDir = normalize( uLightDir ); // light direction points from light to surface

	float ndotl = max( dot( normal, lightDir ), 0.0 );
	vec3 color = uAmbientColor + uDiffuseColor * ndotl;

	FragColor = vec4( color, 1.0 );
}

