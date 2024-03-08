#include <d3d12.h>
#include <wrl.h>
#include "DeviceResources.h"
#pragma once

#define BLOCK_COUNT_X 16
#define BLOCK_COUNT_Y 16
#define BLOCK_COUNT_Z 16


using namespace  Microsoft::WRL;
enum class BlockType
{
	none,
	grass
};

struct Vertex
{
	float x;
	float y;
	float z;
};

struct Chunk
{
	BlockType* blockGrid;
	ComPtr<ID3D12Resource> memoryForBlocksVertecies;
	ComPtr<ID3D12Resource> memoryForBlocksIndecies;
	UINT indexCount;
};

Chunk* CreateChunk(DeviceResources* device);
