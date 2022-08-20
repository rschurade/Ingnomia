#include <bgfx_compute.sh>

// DO NOT CHANGE, must match game internal layout
struct TileData {
	// TF_ flags 0:32
	uint flags;
	// TF_ flags 32:64
	uint flags2;

	// Sprites are all:
	// spriteID=0:16, spriteFlags=16:32
	uint floorSpriteUID;
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

BUFFER_WR(tileData, TileData, 0);
BUFFER_RO(tileDataUpdate, TileDataUpdate, 1);

uniform vec4 uUpdateSize;

NUM_THREADS(64, 1, 1)

void main()
{
	uint id = gl_GlobalInvocationID.x;
	if(id < uUpdateSize.x)
	{
		TileDataUpdate data = tileDataUpdate[id];
		tileData[data.id] = data.tile;
	}
}
