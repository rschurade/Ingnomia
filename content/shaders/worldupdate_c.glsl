#version 430 core

// DO NOT CHANGE, must match game internal layout
struct TileData {
	// TF_ flags 0:32
	uint flags;
	// TF_ flags 32:64
	uint flags2;

	// Sprites are all:
	// spriteID=0:16, spriteFlags=16:32
	uint floorSpriteUID ;
	uint wallSpriteUID;
	
	uint itemSpriteUID;
	uint creatureSpriteUID;

	uint jobSpriteFloorUID;
	uint jobSpriteWallUID;

	// fluidLevel=0:8, lightLevel=0:8, vegetationLevel=8:16
	uint packedLevels;
};

struct TileDataUpdate {
	uint id;
	TileData tile;
};

layout(std430, binding = 0) writeonly restrict buffer tileData1
{
	TileData data[];
} tileData;

layout(std430, binding = 1) readonly restrict buffer tileDataUpdate1
{
	TileDataUpdate data[];
} tileDataUpdate;

uniform int uUpdateSize;

layout(local_size_x = 64) in;

void main()
{
	uint id = gl_GlobalInvocationID.x;
	if(id < uUpdateSize)
	{
		TileDataUpdate data = tileDataUpdate.data[id];
		tileData.data[data.id] = data.tile;
	}
}
