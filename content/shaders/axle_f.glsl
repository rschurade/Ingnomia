#version 430 core

layout(location = 0) noperspective in vec2 vTexCoords;

layout(location = 0) out vec4 fColor;

uniform int uWorldRotation;
uniform int uTickNumber;

uniform int uSpriteID;
uniform int uRotation;
uniform bool uAnim;

uniform sampler2DArray uTexture[32];

vec4 getTexel( uint spriteID, uint rot, uint animFrame )
{
	uint tex = ( spriteID + animFrame ) / 512;
	uint localID = ( ( spriteID + animFrame ) - ( tex * 512 ) ) * 4 + rot;
	
	return texture( uTexture[tex], vec3( vTexCoords, localID  ) );
}

void main()
{
	uint animFrame = 0;
	if( uAnim )
	{
		animFrame = ( uTickNumber / 10 ) % 4;
	}
	
	int rot = ( uRotation + uWorldRotation ) % 4;
	vec4 texel = vec4(0.0);
	
	texel = getTexel( uSpriteID, rot, animFrame ); // TODO add anim 
	
	if( length( texel.rgba ) < 0.1 || texel.a < 0.1 ) 
	{
		discard;
	}
	
	
	
	fColor = texel;
}