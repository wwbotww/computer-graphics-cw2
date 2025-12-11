#version 410 core

in float vAlpha;

uniform sampler2D uParticleTex;
uniform vec3 uParticleColor;

out vec4 FragColor;

void main()
{
	vec4 tex = texture( uParticleTex, gl_PointCoord );
	float alpha = tex.a * vAlpha;
	FragColor = vec4( tex.rgb * uParticleColor, alpha );
}


