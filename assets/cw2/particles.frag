#version 410 core

in float vAlpha;

uniform sampler2D uParticleTex;
uniform vec3 uParticleColor;

out vec4 FragColor;

void main()
{
	vec4 tex = texture( uParticleTex, gl_PointCoord );
	// 使用纹理形状与粒子 alpha 共同控制透明度
	float alpha = tex.a * vAlpha;
	FragColor = vec4( tex.rgb * uParticleColor, alpha );
}


