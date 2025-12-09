#version 410 core

in vec2 vUV;
in vec4 vColor;

uniform sampler2D uTexture;
uniform int uUseTexture;

out vec4 FragColor;

void main()
{
	vec4 base = (uUseTexture != 0) ? texture( uTexture, vUV ) : vec4( 1.0 );
	FragColor = vec4( base.rgb, base.a ) * vColor;
}


