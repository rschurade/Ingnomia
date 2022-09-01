$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2DARRAY(uTexture0, 0);

uniform uint uType;

void main()
{
	vec3 texcoord = vec3( v_texcoord0, uType * 4 );
	vec4 texel = texture2DArray( uTexture0, texcoord );

	if( length( texel.rgba ) < 0.1 || texel.a < 0.1 )
	{
		discard;
	}
	gl_FragColor = texel;
}