#include "Chunk.h"
#include "DeviceResources.h"
#pragma once

struct ChunkGrid
{
	Chunk** gridOfChunks;
	//half of the total number of chunks (exluding 0) in x and z direction
	unsigned int X_halfWidth;
	unsigned int Z_halfWidth;
	unsigned int totalChunks;
	//half of the total number of renderable chunks (exluding 0) in x and z direction
	unsigned int X_halfWidthRender;
	unsigned int Z_halfWidthRender;
	unsigned int totalRenderableChunks;
	unsigned int totalProcessedChunks;
};

ChunkGrid* CreateChunkGrid(DeviceResources* device,
	unsigned int X_halfWidth, unsigned int Z_halfWidth, unsigned int X_halfWidthRender, unsigned int Z_halfWidthRender);

Chunk* GetNthRenderableChunkFromCameraPos(ChunkGrid* chunkGrid, float posX, float posZ, unsigned int index);
XMINT2 GetGridCoordsFromRenderingChunkIndex(ChunkGrid* chunkGrid, float centerX, float centerZ, unsigned int index);
void GenerateChunk(DeviceResources* device, ChunkGrid* chunkGrid, int chunkPosX, int chunkPosZ);
XMINT2 GetCameraPosInGrid(float x, float z);
bool IsInBorder(ChunkGrid* chunkGrid, XMINT2 camCenter, XMINT2 chunkPos);
void UpdateChunkIndexBuffer(DeviceResources* device, ChunkGrid* chunkGrid, int chunkPosX, int chunkPosZ);
