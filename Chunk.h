#include <d3d12.h>
#include <wrl.h>
#include "DeviceResources.h"
#pragma once

#define BLOCK_COUNT_X 16
#define BLOCK_COUNT_Y 128
#define BLOCK_COUNT_Z 16

#define VERTEX_COUNT_X (BLOCK_COUNT_X + 1)
#define VERTEX_COUNT_Y (BLOCK_COUNT_Y + 1)
#define VERTEX_COUNT_Z (BLOCK_COUNT_Z + 1)

#define VERTEX_PER_CUBE 8
#define INDEX_PER_CUBE 36
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

typedef void* DevicePointer;
struct Chunk
{
	BlockType* blockGrid;
	ComPtr<ID3D12Resource> memoryForBlocksVertecies;
	ComPtr<ID3D12Resource> memoryForBlocksIndecies;
	DevicePointer vertexMap;
	DevicePointer indexMap;
	UINT indexCount;

	D3D12_VERTEX_BUFFER_VIEW vertexView;
	D3D12_INDEX_BUFFER_VIEW indexView;
};

Chunk* CreateChunk(DeviceResources* device);
BlockType GetBlockType(Chunk* chunk, unsigned int x, unsigned int y, unsigned int z);
void UpdateGpuMemory(Chunk * chunk);
