#include "Chunk.h"
#include "DeviceResources.h"
#pragma once

struct ChunkGrid
{
	Chunk** gridOfChunks;
	unsigned int X_halfWidth;
	unsigned int Z_halfWidth;
	unsigned int totalChunks;
	unsigned int X_halfWidthRender;
	unsigned int Z_halfWidthRender;
	unsigned int totalRenderableChunks;
};

ChunkGrid* CreateChunkGrid(DeviceResources* device,
	unsigned int X_halfWidth, unsigned int Z_halfWidth, unsigned int X_halfWidthRender, unsigned int Z_halfWidthRender);

Chunk* GetNthRenderableChunkFromCameraPos(ChunkGrid* chunkGrid, float posX, float posZ, unsigned int index);