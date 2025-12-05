#version 410 core

in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vTexCoord;

uniform vec3 uLightDir;
uniform vec3 uAmbientColor;
uniform vec3 uDiffuseColor;
uniform sampler2D uTerrainTexture;

out vec4 FragColor;

void main()
{
	vec3 normal = normalize( vNormal );
	vec3 lightDir = normalize( uLightDir ); // light direction points from light to surface

	float ndotl = max( dot( normal, lightDir ), 0.0 );
	vec3 lighting = uAmbientColor + uDiffuseColor * ndotl;
	vec3 albedo = texture( uTerrainTexture, vTexCoord ).rgb;

	FragColor = vec4( albedo * lighting, 1.0 );
}

