$input v_texcoord0

#include <bgfx_shader.sh>

uniform uint uWorldRotation;
uniform uint uTickNumber;

uniform uint uSpriteID;
uniform uint uRotation;
uniform bool uAnim;

SAMPLER2DARRAY(uTexture, 0);

vec4 getTexel( uint spriteID, uint rot, uint animFrame, vec2 texcoord )
{
	uint absoluteId = ( spriteID + animFrame ) * 4u;
	uint tex = absoluteId / 2048u;
	uint localBaseId = absoluteId % 2048u;
	uint localID = localBaseId + rot;
	
	ivec3 samplePos = ivec3( texcoord.x * 32, texcoord.y * 64, localID);

	return texelFetch( uTexture, samplePos, int(tex) );
}

void main()
{
	int animFrame = 0;
	if( uAnim )
	{
		animFrame = ( uTickNumber / 10 ) % 4;
	}
	
	int rot = ( uRotation + uWorldRotation ) % 4;
	vec4 texel = vec4( 0.0, 0.0, 0.0, 0.0 );
	
	texel = getTexel( uint(uSpriteID), uint(rot), uint(animFrame), v_texcoord0 ); // TODO add anim
	
	if( length( texel.rgba ) < 0.1 || texel.a < 0.1 ) 
	{
		discard;
	}

	gl_FragColor = texel;
}