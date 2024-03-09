#include <d3d12.h>
#include <wrl.h>
#include "DeviceResources.h"
#pragma once

#define BLOCK_COUNT_X 16
#define BLOCK_COUNT_Y 178
#define BLOCK_COUNT_Z 16

#define VERTECIES_PER_CUBE 8
#define INDECIES_PER_CUBE 36
using namespace  Microsoft::WRL;
enum class BlockType
{
	air,
	grass
};

struct Vertex
{
	float x;
	float y;
	float z;
};

struct ChunkCbuffer
{
	float chunkOffsetX;// in x dim
	float chunkOffsetZ;// in z dim
};

typedef void* DevicePointer;
struct Chunk
{
	BlockType* blockGrid;
	ComPtr<ID3D12Resource> memoryForBlocksVertecies;
	ComPtr<ID3D12Resource> memoryForBlocksIndecies;
	ComPtr<ID3D12Resource> cbuffer;
	DevicePointer vertexMap;
	DevicePointer indexMap;
	DevicePointer cbufferMap;
	UINT indexCount;

	D3D12_VERTEX_BUFFER_VIEW vertexView;
	D3D12_INDEX_BUFFER_VIEW indexView;
	ChunkCbuffer cbufferHost;
};

Chunk* CreateChunk(DeviceResources* device, int x_grid_coord = 0, int z_grid_coord = 0);
BlockType GetBlockType(Chunk* chunk, unsigned int x, unsigned int y, unsigned int z);
void UpdateGpuMemory(Chunk * chunk, Chunk* leftNeighbour = nullptr,
					Chunk* rightNeighbour = nullptr, Chunk* backNeighbour = nullptr, Chunk* frontNeighbour = nullptr);
