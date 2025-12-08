#version 410 core

in vec3 vNormal;
in vec3 vWorldPos;
in vec3 vColor;

uniform vec3 uLightDir;
uniform vec3 uAmbientColor;
uniform vec3 uDiffuseColor;

out vec4 FragColor;

void main()
{
	vec3 normal = normalize( vNormal );
	vec3 lightDir = normalize( uLightDir );

	float ndotl = max( dot( normal, lightDir ), 0.0 );
	vec3 lighting = uAmbientColor + uDiffuseColor * ndotl;

	FragColor = vec4( vColor * lighting, 1.0 );
}


