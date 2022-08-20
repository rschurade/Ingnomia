#version 430 core

layout(location = 0) noperspective in vec2 vTexCoords;

layout(location = 0) out vec4 fColor;

uniform sampler2DArray uTexture0;

uniform int uType;

void main()
{
	vec4 texel = texture( uTexture0, vec3( vTexCoords, uType * 4 ) );
	
	if( length( texel.rgba ) < 0.1 || texel.a < 0.1 ) 
	{
		discard;
	}
	fColor = texel;
}