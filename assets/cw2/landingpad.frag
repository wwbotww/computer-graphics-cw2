#version 410 core

in vec3 vNormal;
in vec3 vWorldPos;
in vec3 vColor;

uniform vec3 uLightDir;
uniform vec3 uAmbientColor;
uniform vec3 uDiffuseColor;

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
	vec3 lighting = uAmbientColor;
	if (uDirLightEnabled)  // 方向光可开关
    {
        vec3 lightDir = normalize(uLightDir);
        float ndotl = max(dot(normal, lightDir), 0.0);
        lighting += uDiffuseColor * ndotl;
    }

	for (int i = 0; i < NUM_POINT_LIGHTS; ++i)
    {
        if (!uPointLights[i].enabled)
            continue;

        vec3 L = uPointLights[i].position - vWorldPos;
        float dist = length(L);
        L /= dist;

        // 1/r^2 衰减
        float attenuation = 1.0 / (dist * dist);

        float ndotl = max(dot(normal, L), 0.0);

        lighting += uPointLights[i].color * ndotl * attenuation;
    }

	FragColor = vec4( vColor * lighting, 1.0 );
}


