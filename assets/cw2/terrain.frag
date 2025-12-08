#version 410 core

in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vTexCoord;

uniform vec3 uLightDir;
uniform vec3 uAmbientColor;
uniform vec3 uDiffuseColor;
uniform sampler2D uTerrainTexture;

//task6
struct PointLight
{
    bool enabled;
    vec3 position;
    vec3 color;
};

const int NUM_POINT_LIGHTS = 3;
uniform PointLight uPointLights[NUM_POINT_LIGHTS];
uniform bool uDirLightEnabled;

out vec4 FragColor;

void main()
{
    vec3 normal = normalize( vNormal );
    vec3 albedo = texture( uTerrainTexture, vTexCoord ).rgb;

    vec3 lighting = uAmbientColor;

    if (uDirLightEnabled)
    {
        vec3 lightDir = normalize( uLightDir ); // light direction points from light to surface
        float ndotl = max( dot( normal, lightDir ), 0.0 );
        lighting += uDiffuseColor * ndotl;
    }

    // task6
    for (int i = 0; i < NUM_POINT_LIGHTS; ++i)
    {
        if (!uPointLights[i].enabled)
            continue;

        vec3 L = uPointLights[i].position - vWorldPos;
        float dist = length(L);
        if (dist <= 0.0001)
            continue;

        L /= dist;

        float attenuation = 1.0 / (dist * dist);

        float ndotl = max(dot(normal, L), 0.0);

        lighting += uPointLights[i].color * ndotl * attenuation;
    }

    FragColor = vec4( albedo * lighting, 1.0 );
}
