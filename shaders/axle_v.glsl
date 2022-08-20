#version 430 core

layout(location = 0) in vec3 aPos;

layout(location = 0) noperspective out vec2 vTexCoords;

uniform uvec3 uWorldSize;
uniform mat4 uTransform;
uniform int uWorldRotation;
uniform uvec3 uRenderMin;
uniform uvec3 uRenderMax;

uniform uvec3 tile;

uvec3 rotate(uvec3 pos)
{
	uvec3 ret = uvec3(0, 0, pos.z);
	switch ( uWorldRotation )
	{
		default:
			ret.xy = pos.xy;
		break;
		case 1:
			ret.x = uWorldSize.y - pos.y - 1;
			ret.y = pos.x;
		break;
		case 2:
			ret.x = uWorldSize.x - pos.x - 1;
			ret.y = uWorldSize.y - pos.y - 1;
		break;
		case 3:
			ret.x = pos.y;
			ret.y = uWorldSize.x - pos.x - 1;
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
	float z = pos.z + pos.x + pos.y;
	return vec3(
		offset.x * 32 + 16 * pos.x - 16 * pos.y,
		offset.y * 64 - ( 8 * pos.y + 8 * pos.x ) - ( uRenderMax.z - pos.z ) * 20 - 12,
		isWall ? z + 0.5 : z
	);
}

void main()
{
	vTexCoords = vec2( aPos.x, 1.0 - aPos.y );

	vec3 worldPos = project( rotate( tile ), aPos.xy, true);

	gl_Position = uTransform * vec4( worldPos, 1.0 );
}
