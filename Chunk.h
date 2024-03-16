#include <d3d12.h>
#include <wrl.h>
#include "DeviceResources.h"
#pragma once
#include <directxcollision.h>
#define BLOCK_COUNT_X 16
#define BLOCK_COUNT_Y 178
#define BLOCK_COUNT_Z 16

#define VERTECIES_PER_CUBE 36
#define INDECIES_PER_CUBE 36
#define x_coord_start  -1.0f
#define y_coord_start  ( (float)BLOCK_COUNT_Y / BLOCK_COUNT_X )
#define z_coord_start  ( -(float)BLOCK_COUNT_Z / BLOCK_COUNT_X )

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
	float u;
	float v;
};

struct ChunkCbuffer
{
	float chunkOffsetX;// in x dim
	float chunkOffsetZ;// in z dim
};


struct BoundingVolumeSphere
{
	XMFLOAT3 sphereCenter;
	float sphereRadius;
};
typedef void* DevicePointer;
struct Chunk
{
	static volatile bool isVertexBufferInitialized;
	static ComPtr<ID3D12Resource> memoryForBlocksVertecies;
	static DevicePointer vertexMap;

	BlockType* blockGrid;
	ComPtr<ID3D12Resource> memoryForBlocksIndecies;
	ComPtr<ID3D12Resource> cbuffer;
	DevicePointer indexMap;
	DevicePointer cbufferMap;
	UINT indexCount;

	static D3D12_VERTEX_BUFFER_VIEW vertexView;
	D3D12_INDEX_BUFFER_VIEW indexView;
	ChunkCbuffer cbufferHost;
	std::vector<int> heightMap; // X x Z array
	BoundingVolumeSphere boundingVolume;
	bool drawable;
};

Chunk* CreateChunk(DeviceResources* device, int x_grid_coord = 0, int z_grid_coord = 0, std::vector<int> heightMap = {});
BlockType GetBlockType(Chunk* chunk, unsigned int x, unsigned int y, unsigned int z);
void UpdateGpuMemory(DeviceResources* device, Chunk * chunk, Chunk* leftNeighbour = nullptr,
					Chunk* rightNeighbour = nullptr, Chunk* backNeighbour = nullptr, Chunk* frontNeighbour = nullptr);
void InitMultithreadingResources();