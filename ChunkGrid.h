#include "Chunk.h"
#include "DeviceResources.h"
#pragma once

struct ChunkGrid
{
	Chunk** gridOfChunks;
	unsigned int X_halfWidth;
	unsigned int Z_halfWidth;
	unsigned int totalChunks;
};

ChunkGrid* CreateChunkGrid(DeviceResources* device, unsigned int X_halfWidth, unsigned int Z_halfWidth);