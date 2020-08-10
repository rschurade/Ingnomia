#version 430 core

#define TF_NONE                 0x00000000
#define TF_WALKABLE             0x00000001
#define TF_UNDISCOVERED         0x00000002
#define TF_SUNLIGHT             0x00000004
#define TF_WET                  0x00000008
#define TF_GRASS                0x00000010
#define TF_NOPASS               0x00000020
#define TF_BLOCKED              0x00000040
#define TF_DOOR                 0x00000080
#define TF_STOCKPILE            0x00000100
#define TF_GROVE                0x00000200
#define TF_FARM                 0x00000400
#define TF_TILLED               0x00000800
#define TF_WORKSHOP             0x00001000
#define TF_ROOM                 0x00002000
#define TF_LAVA                 0x00004000
#define TF_WATER                0x00008000
#define TF_JOB_FLOOR            0x00010000
#define TF_JOB_WALL             0x00020000
#define TF_JOB_BUSY_FLOOR       0x00040000
#define TF_JOB_BUSY_WALL        0x00080000
#define TF_MOUSEOVER            0x00100000
#define TF_WALKABLEANIMALS      0x00200000
#define TF_WALKABLEMONSTERS     0x00400000
#define TF_PASTURE              0x00800000
#define TF_INDIRECT_SUNLIGHT    0x01000000

#define WATER_TOP               0x01
#define WATER_EDGE              0x02
#define WATER_WALL              0x10
#define WATER_FLOOR             0x20
#define WATER_ONFLOOR           0x40

#define CAT(x, y) CAT_(x, y)
#define CAT_(x, y) x ## y
#define UNPACKSPRITE(alias, src) const uint CAT(alias, ID) = src & 0xffff; const uint CAT(alias, Flags) = src >> 16;

layout(location = 0) noperspective in vec2 vTexCoords;
layout(location = 1) flat in uvec4  block1;
layout(location = 2) flat in uvec4  block2;
layout(location = 3) flat in uvec4  block3;

layout(location = 0) out vec4 fColor;

uniform sampler2DArray uTexture[32];

uniform int uTickNumber;

uniform int uUndiscoveredTex;
uniform int uWaterTex;

uniform int uWorldRotation;
uniform bool uOverlay;
uniform bool uDebug;
uniform bool uDebugOverlay;
uniform bool uWallsLowered;
uniform float uDaylight;
uniform float uLightMin;
uniform bool uPaintFrontToBack;

const float waterAlpha = 0.6;
const float flSize =  ( 1.0 / 32. );
const int rightWallOffset = 4;
const int leftWallOffset = 8;

const vec3 perceivedBrightness = vec3(0.299, 0.587, 0.114);

vec4 getTexel( uint spriteID, uint rot, uint animFrame )
{
	uint tex = ( spriteID + animFrame ) / 512;
	uint localID = ( ( spriteID + animFrame ) - ( tex * 512 ) ) * 4 + rot;
	
	return texture( uTexture[tex], vec3( vTexCoords, localID  ) );
}

void main()
{
	vec4 texel = vec4(0.0);
	
	uint rot = 0;
	uint spriteID = 0;
	uint animFrame = 0;
	
	UNPACKSPRITE(floorSprite, block1.x);
	UNPACKSPRITE(jobFloorSprite, block1.y);
	UNPACKSPRITE(wallSprite, block1.z);
	UNPACKSPRITE(jobWallSprite, block1.w);
	UNPACKSPRITE(itemSprite, block2.x);
	UNPACKSPRITE(creatureSprite, block2.y);

	const uint vFluidLevelPacked1 = block2.z;
	const bool uIsWall = ( block2.w != 0 );

	const uint vFlags = block3.x;
	const uint vFlags2 = block3.y;

	const uint vLightLevel = block3.z;
	const uint vVegetationLevel = block3.w;

	uint vFluidLevel = (vFluidLevelPacked1 >> 0) & 0xff;
	uint vFluidLevelLeft = (vFluidLevelPacked1 >> 8) & 0xff;
	uint vFluidLevelRight = (vFluidLevelPacked1 >> 16) & 0xff;
	uint vFluidFlags = (vFluidLevelPacked1 >> 24) & 0xff;

	
	if( !uIsWall )
	{
		if( ( vFlags & TF_UNDISCOVERED ) != 0 && !uDebug )
		{
			if( !uWallsLowered )
			{
				texel = texture( uTexture[0], vec3( vTexCoords, uUndiscoveredTex ) );
			}
		}
		else
		{
			spriteID = floorSpriteID;
			if( spriteID > 0 )
			{
				rot = floorSpriteFlags & 3;
				rot = ( rot + uWorldRotation ) % 4;
				
				if( ( floorSpriteFlags & 4 ) == 4 )
				{
					animFrame = ( uTickNumber / 10 ) % 4;
				}
				
				texel = getTexel( spriteID, rot, animFrame );
				
				if( ( vFlags & TF_GRASS ) != 0 )
				{
					vec4 roughFloor = texture( uTexture[0], vec3( vTexCoords, uUndiscoveredTex + 12 )  );
					float interpol = 1.0 - ( float( vVegetationLevel ) / 100. );
					texel = mix( texel, roughFloor, interpol );
				}
			}
			
			spriteID = jobFloorSpriteID;
			animFrame = 0;
			if( ( spriteID > 0 )  )
			{
				rot = jobFloorSpriteFlags & 3;
				rot = ( rot + uWorldRotation ) % 4;
				
				vec4 tmpTexel = getTexel( spriteID, rot, animFrame );
				
				if( ( vFlags & TF_JOB_BUSY_FLOOR ) != 0 )
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
				texel += tmpTexel;
			}

			if( uOverlay && 0 != ( vFlags & ( TF_STOCKPILE | TF_FARM | TF_GROVE | TF_PASTURE | TF_WORKSHOP | TF_ROOM | TF_NOPASS ) ) )
			{
				vec3 roomColor = vec3( 0.0 );
			
				if( ( vFlags & TF_STOCKPILE ) > 0 ) //stockpile
				{
					roomColor = vec3(1, 1, 0);
				}
				
				else if( ( vFlags & TF_FARM ) > 0 ) //farm
				{
					roomColor = vec3(0.5, 0, 1);
				}
				else if( ( vFlags & TF_GROVE ) > 0 ) //grove
				{
					roomColor = vec3(0, 1, 0.5);
				}
				else if( ( vFlags & TF_PASTURE ) > 0 ) 
				{
					roomColor = vec3(0, 0.9, 0.9);
				}
				else if( ( vFlags & TF_WORKSHOP ) > 0 ) //workshop
				{
					if( ( vFlags & TF_BLOCKED ) > 0 )
					{
						roomColor = vec3(1, 0, 0);
					}
					else
					{
						roomColor = vec3(1, 1, 0);
					}
				}
				else if( ( vFlags & TF_ROOM ) > 0 ) //room
				{
					roomColor = vec3(0, 0, 1);
				}
				else if( ( vFlags & TF_NOPASS ) > 0 ) //room
				{
					roomColor = vec3(1, 0, 0);
				}
				if( texel.a != 0 )
				{
					float brightness = dot(texel.rgb, perceivedBrightness.xyz);
					texel.rgb = mix( roomColor, mix( texel.rgb, vec3(1,1,1) * brightness, 0.7), 0.7);
				}
				
			}
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// water related calculations
		//
		////////////////////////////////////////////////////////////////////////////////////////////////////
		if( ( vFluidFlags & ( WATER_FLOOR | WATER_EDGE ) ) != 0 )
		{
			const bool renderAboveFloor = ( vFluidFlags & WATER_ONFLOOR ) != 0;
			const int startLevel =  renderAboveFloor ? 2 : int(min(vFluidLevel, 2));
			const int referenceLevel = int(vTexCoords.x < 0.5 ? vFluidLevelLeft : vFluidLevelRight);
			const int offset = vTexCoords.x < 0.5 ? leftWallOffset : rightWallOffset;

			const float fl = float( startLevel - 2 ) * flSize;

			vec4 tmpTexel = vec4( 0 );

			if( ( vFluidFlags & WATER_FLOOR ) != 0 )
			{
				float y = vTexCoords.y + fl;
				tmpTexel = texture( uTexture[0], vec3( vec2( vTexCoords.x, y ), uWaterTex ) );
			}

			if( ( vFluidFlags & WATER_EDGE ) != 0 )
			{
				float y = vTexCoords.y + fl;
				for(int i = startLevel; i > referenceLevel; --i)
				{
					y -= flSize;
					tmpTexel += texture( uTexture[0], vec3( vec2( vTexCoords.x, y ), uWaterTex + offset ) );
				}
			}

			// Turn into slight tint instead
			if( renderAboveFloor && vFluidLevel == 1 )
			{
				tmpTexel.a *= 0.5;
			}

			if( texel.a != 0 ) 
			{
				texel.rgb = mix( texel.rgb, tmpTexel.rgb, waterAlpha * tmpTexel.a );
				texel.a = max(texel.a , tmpTexel.a);
			}
			else
			{
				texel = tmpTexel;
			}
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// end water related calculations
		//
		////////////////////////////////////////////////////////////////////////////////////////////////////
		
		
	}
	else
	{
		if( ( vFlags & TF_UNDISCOVERED ) != 0 && !uDebug )
		{
			if( !uWallsLowered )
			{
				texel = texture( uTexture[0], vec3( vTexCoords, uUndiscoveredTex ) );
			}
		}
		else
		{
			spriteID = wallSpriteID;
			if( spriteID > 0 )
			{
				rot = wallSpriteFlags & 3;
				rot = ( rot + uWorldRotation ) % 4;

				if( ( wallSpriteFlags & 4 ) == 4 )
				{
					animFrame = ( uTickNumber / 3 ) % 4;
				}
				if( uWallsLowered )
				{
					animFrame = 0;
					if( ( wallSpriteFlags & 8 ) == 8 )
					{
						spriteID = spriteID + 1;
					}
				}
				vec4 tmpTexel = getTexel( spriteID, rot, animFrame );
				
				if( tmpTexel.a != 0 ) 
				{
					texel = tmpTexel;
				}
			}
			
			
			spriteID = itemSpriteID;
			animFrame = 0;
			if( spriteID > 0 )
			{
				rot = itemSpriteID & 3;
				rot = ( rot + uWorldRotation ) % 4;
				
				vec4 tmpTexel = getTexel( spriteID, rot, animFrame );
				
				if( tmpTexel.a != 0 ) 
				{
					texel = tmpTexel;
				}
			}
		}
	
	
		spriteID = jobWallSpriteID;
		animFrame = 0;
		if( spriteID > 0 && ( vFlags & TF_JOB_WALL ) > 0 )
		{
			rot = jobWallSpriteFlags & 3;
			rot = ( rot + uWorldRotation ) % 4;
			
			vec4 tmpTexel = getTexel( spriteID, rot, animFrame );
			
			if( ( vFlags & TF_JOB_BUSY_WALL ) != 0 )
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
			if( texel.a != 0 ) 
			{
				texel.rgb = mix( texel.rgb, tmpTexel.rgb, tmpTexel.a );
				texel.a = max(texel.a , tmpTexel.a);
			}
			else
			{
				texel = tmpTexel;
			}
		}

	
		spriteID = creatureSpriteID;
		animFrame = 0;
		if( spriteID > 0 )
		{
			rot = creatureSpriteFlags & 3;
			rot = ( rot + uWorldRotation ) % 4;
			
			vec4 tmpTexel = getTexel( spriteID, rot, animFrame );
			
			if( tmpTexel.a != 0 ) 
			{
				texel = tmpTexel;
			}
		}
		
		
		////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// water related calculations
		//
		////////////////////////////////////////////////////////////////////////////////////////////////////
		if( ( vFluidFlags & ( WATER_TOP | WATER_WALL ) ) != 0 && vFluidLevel > 2 )
		{
			const int startLevel = int(vFluidLevel - 2);
			const int referenceLevel = int( vTexCoords.x < 0.5 ? max(2, vFluidLevelLeft) : max(2, vFluidLevelRight) ) - 2;
			const int offset = vTexCoords.x < 0.5 ? leftWallOffset : rightWallOffset;

			const float fl = float( startLevel ) * flSize;

			vec4 tmpTexel = vec4( 0 );
			
			if( ( vFluidFlags & WATER_TOP ) != 0 )
			{
				float y = vTexCoords.y + fl;
				tmpTexel = texture( uTexture[0], vec3( vec2( vTexCoords.x, y ), uWaterTex ) );
			}

			if( ( vFluidFlags & WATER_WALL ) != 0)
			{
				float y = vTexCoords.y + fl;
				for(int i = startLevel; i > referenceLevel; --i)
				{
					y -= flSize;
					tmpTexel += texture( uTexture[0], vec3( vec2( vTexCoords.x, y ), uWaterTex + offset ) );
				}
			}
			
			if( texel.a != 0 ) 
			{
				texel.rgb = mix( texel.rgb, tmpTexel.rgb, waterAlpha * tmpTexel.a );
				texel.a = max(texel.a , tmpTexel.a);
			}
			else
			{
				texel = tmpTexel;
			}
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// end water related calculations
		//
		////////////////////////////////////////////////////////////////////////////////////////////////////
	}

	if( texel.a == 0 ) 
	{
		discard;
	}
	else if(uPaintFrontToBack)
	{
		// Flush to 1 in case of front-to-back rendering
		texel.a == 1;
	}
	
	
	if( !uDebug )
	{
		float light = float( vLightLevel ) / 20.;
		if( ( vFlags & ( TF_SUNLIGHT | TF_INDIRECT_SUNLIGHT ) ) != 0 )
		{
			light = max( light , uDaylight );
		}
		float brightness = dot(texel.rgb, perceivedBrightness.xyz);
		float lightMult = ( 1 - uLightMin ) * light + uLightMin;
		const float minSaturation = 0.1;
		float saturation = ( 1 - minSaturation ) * light + minSaturation;
		// Desaturate, then darken
		texel.rgb = mix(brightness * vec3(1,1,1), texel.rgb, saturation) * lightMult;
	}
	fColor = texel;
}
