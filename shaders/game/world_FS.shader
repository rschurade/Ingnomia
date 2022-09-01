$input v_texcoord0
// This was block1, block2, block3
$input v_texcoord1, v_texcoord2, v_texcoord3

#include <bgfx_shader.sh>

//layout(location = 0) noperspective in vec2 vTexCoords;
//layout(location = 1) flat in uvec4  block1;
//layout(location = 2) flat in uvec4  block2;
//layout(location = 3) flat in uvec4  block3;

//layout(location = 0) out vec4 fColor;

#define TF_NONE                 0x00000000u
#define TF_WALKABLE             0x00000001u
#define TF_UNDISCOVERED         0x00000002u
#define TF_SUNLIGHT             0x00000004u
#define TF_WET                  0x00000008u
#define TF_GRASS                0x00000010u
#define TF_NOPASS               0x00000020u
#define TF_BLOCKED              0x00000040u
#define TF_DOOR                 0x00000080u
#define TF_STOCKPILE            0x00000100u
#define TF_GROVE                0x00000200u
#define TF_FARM                 0x00000400u
#define TF_TILLED               0x00000800u
#define TF_WORKSHOP             0x00001000u
#define TF_ROOM                 0x00002000u
#define TF_LAVA                 0x00004000u
#define TF_WATER                0x00008000u
#define TF_JOB_FLOOR            0x00010000u
#define TF_JOB_WALL             0x00020000u
#define TF_JOB_BUSY_FLOOR       0x00040000u
#define TF_JOB_BUSY_WALL        0x00080000u
#define TF_MOUSEOVER            0x00100000u
#define TF_WALKABLEANIMALS      0x00200000u
#define TF_WALKABLEMONSTERS     0x00400000u
#define TF_PASTURE              0x00800000u
#define TF_INDIRECT_SUNLIGHT    0x01000000u
#define TF_TRANSPARENT          0x40000000u
#define TF_OVERSIZE             0x80000000u

#define WATER_TOP               0x01u
#define WATER_EDGE              0x02u
#define WATER_WALL              0x10u
#define WATER_FLOOR             0x20u
#define WATER_ONFLOOR           0x40u

#define CAT(x, y) CAT_(x, y)
#define CAT_(x, y) x ## y
#define UNPACKSPRITE(alias, src) uint CAT(alias, ID) = src & 0xffffu; uint CAT(alias, Flags) = src >> 16u;

SAMPLER2DARRAY(uTexture, 0);

uniform uint uTickNumber;

uniform uint uUndiscoveredTex;
uniform uint uWaterTex;

uniform uint uWorldRotation;
uniform bool uOverlay;
uniform bool uDebug;
uniform bool uWallsLowered;
uniform float uDaylight;
uniform float uLightMin;
uniform bool uPaintFrontToBack;

uniform bool uShowJobs;

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
	const float waterAlpha = 0.6;
	const float flSize =  ( 1.0 / 32. );
	const int rightWallOffset = 4;
	const int leftWallOffset = 8;

	const vec3 perceivedBrightness = vec3(0.299, 0.587, 0.114);

	vec4 texel = vec4( 0,  0,  0, 0 );
	
	uint rot = 0u;
	uint spriteID = 0u;
	uint animFrame = 0u;
	
	UNPACKSPRITE(floorSprite, v_texcoord1.x);
	UNPACKSPRITE(jobFloorSprite, v_texcoord1.y);
	UNPACKSPRITE(wallSprite, v_texcoord1.z);
	UNPACKSPRITE(jobWallSprite, v_texcoord1.w);
	UNPACKSPRITE(itemSprite, v_texcoord2.x);
	UNPACKSPRITE(creatureSprite, v_texcoord2.y);

	uint vFluidLevelPacked1 = v_texcoord2.z;
	bool uIsWall = ( v_texcoord2.w != 0u );

	uint vFlags = v_texcoord3.x;
	uint vFlags2 = v_texcoord3.y;

	uint vLightLevel = v_texcoord3.z;
	uint vVegetationLevel = v_texcoord3.w;

	uint vFluidLevel = (vFluidLevelPacked1 >> 0u) & 0xffu;
	uint vFluidLevelLeft = (vFluidLevelPacked1 >> 8u) & 0xffu;
	uint vFluidLevelRight = (vFluidLevelPacked1 >> 16u) & 0xffu;
	uint vFluidFlags = (vFluidLevelPacked1 >> 24u) & 0xffu;

	
	if( !uIsWall )
	{
		if( ( vFlags & TF_UNDISCOVERED ) != 0u && !uDebug )
		{
			if( !uWallsLowered )
			{
				vec4 tmpTexel = getTexel( uint(uUndiscoveredTex / 4 + 2), 0u, 0u, v_texcoord0 );

				texel.rgb = mix( texel.rgb, tmpTexel.rgb, tmpTexel.a );
				texel.a = max(texel.a , tmpTexel.a);
			}
		}
		else
		{
			spriteID = floorSpriteID;
			if( spriteID != 0u )
			{
				rot = floorSpriteFlags & 3u;
				rot = uint( int(rot) + uWorldRotation ) % 4u;
				
				if( ( floorSpriteFlags & 4u ) == 4u )
				{
					animFrame = uint( uTickNumber / 10 ) % 4u;
				}
				
				vec4 tmpTexel = getTexel( spriteID, rot, animFrame, v_texcoord0 );
				
				if( ( vFlags & TF_GRASS ) != 0u )
				{
					vec4 roughFloor = getTexel( uint(uUndiscoveredTex / 4 + 3), 0u, 0u, v_texcoord0 );
					float interpol = 1.0 - ( float( vVegetationLevel ) / 100. );
					tmpTexel.rgb = mix( tmpTexel.rgb, roughFloor.rgb, interpol * roughFloor.a );
					texel.a = max(tmpTexel.a , roughFloor.a);
				}

				texel.rgb = mix( texel.rgb, tmpTexel.rgb, tmpTexel.a );
				texel.a = max(texel.a , tmpTexel.a);
			}
			
			spriteID = jobFloorSpriteID;
			animFrame = 0u;
			if( uShowJobs && ( spriteID != 0u )  )
			{
				rot = jobFloorSpriteFlags & 3u;
				rot = uint( int(rot) + uWorldRotation ) % 4u;
				
				vec4 tmpTexel = getTexel( spriteID, rot, animFrame, v_texcoord0 );
				
				if( ( vFlags & TF_JOB_BUSY_FLOOR ) != 0u )
				{
					tmpTexel.r *= 0.3;
					tmpTexel.g *= 0.7;
					tmpTexel.b *= 0.3;
				}
				else
				{
					tmpTexel.r *= 0.7;
					tmpTexel.g *= 0.7;
					tmpTexel.b *= 0.3;
				}

				texel.rgba = tmpTexel.rgba;
			}

			if( uOverlay && 0u != ( vFlags & ( TF_STOCKPILE | TF_FARM | TF_GROVE | TF_PASTURE | TF_WORKSHOP | TF_ROOM | TF_NOPASS ) ) )
			{
				vec3 roomColor = vec3( 0, 0, 0 );
			
				if( ( vFlags & TF_STOCKPILE ) != 0u ) //stockpile
				{
					roomColor = vec3(1, 1, 0);
				}
				
				else if( ( vFlags & TF_FARM ) != 0u ) //farm
				{
					roomColor = vec3(0.5, 0, 1);
				}
				else if( ( vFlags & TF_GROVE ) != 0u ) //grove
				{
					roomColor = vec3(0, 1, 0.5);
				}
				else if( ( vFlags & TF_PASTURE ) != 0u )
				{
					roomColor = vec3(0, 0.9, 0.9);
				}
				else if( ( vFlags & TF_WORKSHOP ) != 0u ) //workshop
				{
					if( ( vFlags & TF_BLOCKED ) != 0u )
					{
						roomColor = vec3(1, 0, 0);
					}
					else
					{
						roomColor = vec3(1, 1, 0);
					}
				}
				else if( ( vFlags & TF_ROOM ) != 0u ) //room
				{
					roomColor = vec3(0, 0, 1);
				}
				else if( ( vFlags & TF_NOPASS ) != 0u ) //room
				{
					roomColor = vec3(1, 0, 0);
				}
				if( texel.a != 0 )
				{
					float brightness = dot(texel.rgb, perceivedBrightness.xyz);
					// Preserve perceived brightness of original pixel during tinting, but drop saturation partially
					texel.rgb = mix( roomColor, mix( texel.rgb, mul(vec3(1,1,1), brightness), 0.7), 0.7);
				}
				
			}
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// water related calculations
		//
		////////////////////////////////////////////////////////////////////////////////////////////////////
		if( ( vFluidFlags & ( WATER_FLOOR | WATER_EDGE ) ) != 0u )
		{
			bool renderAboveFloor = ( vFluidFlags & WATER_ONFLOOR ) != 0u;
			int startLevel =  renderAboveFloor ? 2 : int(min(vFluidLevel, 2));
			int referenceLevel = int(v_texcoord0.x < 0.5 ? vFluidLevelLeft : vFluidLevelRight);
			int offset = v_texcoord0.x < 0.5 ? leftWallOffset : rightWallOffset;

			float fl = float( startLevel - 2 ) * flSize;

			vec4 tmpTexel = vec4( 0, 0, 0, 0 );

			if( ( vFluidFlags & WATER_FLOOR ) != 0u )
			{
				float y = v_texcoord0.y + fl;
				tmpTexel = texture2DArray( uTexture, vec3( vec2( v_texcoord0.x, y ), uWaterTex ) );
			}

			if( ( vFluidFlags & WATER_EDGE ) != 0u )
			{
				float y = v_texcoord0.y + fl;
				for(int i = startLevel; i > referenceLevel; --i)
				{
					y -= flSize;
					tmpTexel += texture2DArray( uTexture, vec3( vec2( v_texcoord0.x, y ), uWaterTex + offset ) );
				}
			}

			// Turn into slight tint instead
			if( renderAboveFloor && vFluidLevel == 1u )
			{
				tmpTexel.a *= 0.5;
			}


			texel.rgb = mix( texel.rgb, tmpTexel.rgb, waterAlpha * tmpTexel.a );
			texel.a = max(texel.a , tmpTexel.a);
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// end water related calculations
		//
		////////////////////////////////////////////////////////////////////////////////////////////////////
		
		
	}
	else
	{
		if( ( vFlags & TF_UNDISCOVERED ) != 0u && !uDebug )
		{
			if( !uWallsLowered )
			{
				vec4 tmpTexel = getTexel( uint(uUndiscoveredTex / 4), 0u, 0u, v_texcoord0 );

				texel.rgb = mix( texel.rgb, tmpTexel.rgb, tmpTexel.a );
				texel.a = max(texel.a , tmpTexel.a);
			}
		}
		else
		{
			spriteID = wallSpriteID;
			if( spriteID != 0u )
			{
				rot = wallSpriteFlags & 3u;
				rot = uint( int(rot) + uWorldRotation ) % 4u;

				if( ( wallSpriteFlags & 4u ) == 4u )
				{
					animFrame = uint( uTickNumber / 3 ) % 4u;
				}
				if( uWallsLowered )
				{
					animFrame = 0u;
					if( ( wallSpriteFlags & 8u ) == 8u )
					{
						spriteID = spriteID + 1u;
					}
				}
				vec4 tmpTexel = getTexel( spriteID, rot, animFrame, v_texcoord0 );
				
				texel.rgb = mix( texel.rgb, tmpTexel.rgb, tmpTexel.a );
				texel.a = max(texel.a , tmpTexel.a);
			}
			
			spriteID = itemSpriteID;
			animFrame = 0u;
			if( spriteID != 0u )
			{
				rot = itemSpriteID & 3u;
				rot = uint( int(rot) + uWorldRotation ) % 4u;
				
				vec4 tmpTexel = getTexel( spriteID, rot, animFrame, v_texcoord0 );
				
				texel.rgb = mix( texel.rgb, tmpTexel.rgb, tmpTexel.a );
				texel.a = max(texel.a , tmpTexel.a);
			}
		}
	
		spriteID = jobWallSpriteID;
		animFrame = 0u;
		if( uShowJobs && spriteID != 0u )
		{
			rot = jobWallSpriteFlags & 3u;
			rot = uint( int(rot) + uWorldRotation ) % 4u;
			
			vec4 tmpTexel = getTexel( spriteID, rot, animFrame, v_texcoord0 );
			
			if( ( vFlags & TF_JOB_BUSY_WALL ) != 0u )
			{
				tmpTexel.r *= 0.3;
				tmpTexel.g *= 0.7;
				tmpTexel.b *= 0.3;
			}
			else
			{
				tmpTexel.r *= 0.7;
				tmpTexel.g *= 0.7;
				tmpTexel.b *= 0.3;
			}

			texel.rgba = tmpTexel.rgba;
		}

	
		spriteID = creatureSpriteID;
		animFrame = 0u;
		if( spriteID != 0u )
		{
			rot = creatureSpriteFlags & 3u;
			rot = uint( int(rot) + uWorldRotation ) % 4u;
			
			vec4 tmpTexel = getTexel( spriteID, rot, animFrame, v_texcoord0 );
			
			texel.rgb = mix( texel.rgb, tmpTexel.rgb, tmpTexel.a );
			texel.a = max(texel.a , tmpTexel.a);
		}
		
		////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// water related calculations
		//
		////////////////////////////////////////////////////////////////////////////////////////////////////
		if ( ( vFluidFlags & ( WATER_TOP | WATER_WALL ) ) != 0u && vFluidLevel > 2u )
		{
			int startLevel = int(vFluidLevel - 2u);
			int referenceLevel = int( v_texcoord0.x < 0.5 ? max(2, vFluidLevelLeft) : max(2, vFluidLevelRight) ) - 2;
			int offset = v_texcoord0.x < 0.5 ? leftWallOffset : rightWallOffset;

			float fl = float( startLevel ) * flSize;

			vec4 tmpTexel = vec4( 0, 0, 0, 0 );
			
			if( ( vFluidFlags & WATER_TOP ) != 0u )
			{
				float y = v_texcoord0.y + fl;
				tmpTexel = texture2DArray( uTexture, vec3( vec2( v_texcoord0.x, y ), uWaterTex ) );
			}

			if( ( vFluidFlags & WATER_WALL ) != 0u )
			{
				float y = v_texcoord0.y + fl;
				for(int i = startLevel; i > referenceLevel; --i)
				{
					y -= flSize;
					tmpTexel += texture2DArray( uTexture, vec3( vec2( v_texcoord0.x, y ), uWaterTex + offset ) );
				}
			}
			
			texel.rgb = mix( texel.rgb, tmpTexel.rgb, waterAlpha * tmpTexel.a );
			texel.a = max(texel.a , tmpTexel.a);
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// end water related calculations
		//
		////////////////////////////////////////////////////////////////////////////////////////////////////
	}

	if( texel.a <= 0 )
	{
		discard;
	}
	else if(uPaintFrontToBack)
	{
		// Flush to 1 in case of front-to-back rendering
		texel.a = 1;
	}
	
	
	if( !uDebug )
	{
		float light = float( vLightLevel ) / 20.;
		if( ( vFlags & ( TF_SUNLIGHT | TF_INDIRECT_SUNLIGHT ) ) != 0u )
		{
			light = max( light , uDaylight );
		}
		float brightness = dot(texel.rgb, perceivedBrightness.xyz);
		float lightMult = ( 1 - uLightMin ) * light + uLightMin;
		const float minSaturation = 0.1;
		float saturation = ( 1 - minSaturation ) * light + minSaturation;
		// Desaturate, then darken
		texel.rgb = mul(mix(mul(brightness, vec3(1,1,1)), texel.rgb, saturation), lightMult);
	}
	gl_FragColor = texel;
}
