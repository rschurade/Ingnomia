$input a_position

//layout(location = 0) noperspective out vec2 vTexCoords;
$output v_texcoord0

#include <bgfx_shader.sh>

uniform mat4 uTransform;
uniform uvec4 uWorldSize;
uniform uvec4 uRenderMin;
uniform uvec4 uRenderMax;

uniform uvec4 tile;

uniform int uWorldRotation;

uvec3 rotate(uvec4 pos)
{
	uvec3 ret = uvec3(0, 0, pos.z);
	switch ( uWorldRotation )
	{
		default:
			ret.xy = pos.xy;
		break;
		case 1:
			ret.x = uWorldSize.y - pos.y - 1u;
			ret.y = pos.x;
		break;
		case 2:
			ret.x = uWorldSize.x - pos.x - 1u;
			ret.y = uWorldSize.y - pos.y - 1u;
		break;
		case 3:
			ret.x = pos.y;
			ret.y = uWorldSize.x - pos.x - 1u;
		break;
	}
	return ret;
}

uvec3 rotateOffset(uvec3 offset)
{
	switch ( uWorldRotation )
	{
		default:
			return offset;
		case 1:
			return uvec3( -offset.y, offset.x, offset.z );
		case 2:
			return uvec3( -offset.x, -offset.y, offset.z );
		case 3:
			return uvec3( offset.y, -offset.y, offset.z );
	}
}

vec3 project(uvec3 pos, vec2 offset, bool isWall)
{
	float z = float(pos.z + pos.x + pos.y);
	return vec3(
		offset.x * 32.0 + float(16u * pos.x - 16u * pos.y),
		offset.y * 64.0 - float(( 8u * pos.y + 8u * pos.x ) - ( uRenderMax.z - pos.z ) * 20u - 12u),
		isWall ? z + 0.5 : z
	);
}

void main()
{
	v_texcoord0 = vec2( a_position.x, 1.0 - a_position.y );

	vec3 worldPos = project( rotate( tile ), a_position.xy, true);

	// Offset relative to owning creature
	worldPos.x += 8.0;
	worldPos.y += 16.0;

	gl_Position = mul(uTransform, vec4( worldPos, 1.0 ));
}
