#include "Chunk.h"
#include <random>

/*
Chunk starts at (-X, Y, -Z) ie backward top leftmost corner
y=0 means is top of the chunk

*/
#define FRONT_S (1.0f/3.0f)
#define FRONT_E (2.0f/3.0f)
#define RIGHT_S (1.0f/3.0f)
#define RIGHT_E (2.0f/3.0f)
#define LEFT_S (1.0f/3.0f)
#define LEFT_E (2.0f/3.0f)
#define BACK_S (1.0f/3.0f)
#define BACK_E (2.0f/3.0f)

#define TOP_S 0.6726f
#define TOP_E (1.0f)

#define BOTTOM_S 0 
#define BOTTOM_E (1.0f/3.0f)

constexpr float x_coord_start = -1.0f;
constexpr float y_coord_start = (float)BLOCK_COUNT_Y / BLOCK_COUNT_X;
constexpr float z_coord_start = -(float)BLOCK_COUNT_Z / BLOCK_COUNT_X;

bool Chunk::isVertexBufferInitialized = false;
ComPtr<ID3D12Resource> Chunk::memoryForBlocksVertecies;
D3D12_VERTEX_BUFFER_VIEW Chunk::vertexView = {};
DevicePointer Chunk::vertexMap = nullptr;

static Vertex getVertex(int x, int y, int z, float u, float v)
{
    Vertex vertex = {};
    vertex.x = x_coord_start + 2.0f * abs(x_coord_start) * x / BLOCK_COUNT_X;
    vertex.y = y_coord_start - 2.0f * abs(y_coord_start) * y / BLOCK_COUNT_Y;
    vertex.z = z_coord_start + 2.0f * abs(z_coord_start) * z / BLOCK_COUNT_Z;
    vertex.u = u;
    vertex.v = v;
    return vertex;
}
static void buildMesh(Chunk* chunk)
{
    Vertex* buffer = new Vertex[BLOCK_COUNT_X * BLOCK_COUNT_Y * BLOCK_COUNT_Z * VERTECIES_PER_CUBE];
    for (int z = 0; z < BLOCK_COUNT_Z; z++)
    {
        for (int y = 0; y < BLOCK_COUNT_Y; y++)
        {
            for (int x = 0; x < BLOCK_COUNT_X; x++)
            {
                Vertex vertex = {};
                unsigned int block_id = z * BLOCK_COUNT_Y * BLOCK_COUNT_X + y * BLOCK_COUNT_X + x;
                unsigned int block_index = VERTECIES_PER_CUBE * block_id;
                //front face of cube ----------------------------------------------------------
                buffer[block_index + 0] = getVertex(x,y,z, FRONT_S, 0);
                buffer[block_index + 1] = getVertex(x+1, y, z, FRONT_E, 0);
                buffer[block_index + 2] = getVertex(x, y + 1, z, FRONT_S, 1.0f);

                buffer[block_index + 3] = getVertex(x + 1, y, z, FRONT_E, 0);
                buffer[block_index + 4] = getVertex(x + 1, y + 1, z, FRONT_E, 1.0f);
                buffer[block_index + 5] = getVertex(x, y + 1, z, FRONT_S, 1.0f);
                //right face of cube ----------------------------------------------------------
                block_index += 6;
                buffer[block_index + 0] = getVertex(x + 1,y,z, RIGHT_S, 0);
                buffer[block_index + 1] = getVertex(x + 1, y, z + 1, RIGHT_E, 0);
                buffer[block_index + 2] = getVertex(x + 1, y + 1, z, RIGHT_S, 1.0f);

                buffer[block_index + 3] = getVertex(x + 1, y, z + 1, RIGHT_E, 0);
                buffer[block_index + 4] = getVertex(x + 1, y + 1, z +1, RIGHT_E, 1.0f);
                buffer[block_index + 5] = getVertex(x + 1, y + 1, z, RIGHT_S, 1.0f);
                //left face of cube -----------------------------------------------------------
                block_index += 6;
                buffer[block_index + 0] = getVertex(x, y, z + 1, LEFT_S, 0);
                buffer[block_index + 1] = getVertex(x, y, z, LEFT_E, 0);
                buffer[block_index + 2] = getVertex(x, y + 1, z + 1, LEFT_S, 1.0f);

                buffer[block_index + 3] = getVertex(x, y, z, LEFT_E, 0);
                buffer[block_index + 4] = getVertex(x, y + 1, z, LEFT_E, 1.0f);
                buffer[block_index + 5] = getVertex(x, y + 1, z + 1, LEFT_S, 1.0f);
                //top face of cube ------------------------------------------------------------
                block_index += 6;
                buffer[block_index + 0] = getVertex(x, y, z + 1, TOP_S, 0);
                buffer[block_index + 1] = getVertex(x + 1, y, z + 1, TOP_E, 0);
                buffer[block_index + 2] = getVertex(x, y, z, TOP_S, 1.0f);

                buffer[block_index + 3] = getVertex(x + 1, y, z + 1, TOP_E, 0);
                buffer[block_index + 4] = getVertex(x + 1, y, z, TOP_E, 1.0f);
                buffer[block_index + 5] = getVertex(x, y, z, TOP_S, 1.0f);
                //bottom face of cube ---------------------------------------------------------
                block_index += 6;
                buffer[block_index + 0] = getVertex(x, y + 1, z, BOTTOM_S, 0);
                buffer[block_index + 1] = getVertex(x + 1, y + 1, z, BOTTOM_E, 0);
                buffer[block_index + 2] = getVertex(x, y + 1, z + 1, BOTTOM_S, 1.0f);

                buffer[block_index + 3] = getVertex(x + 1, y + 1, z, BOTTOM_E, 0);
                buffer[block_index + 4] = getVertex(x + 1, y + 1, z + 1, BOTTOM_E, 1.0f);
                buffer[block_index + 5] = getVertex(x, y + 1, z + 1, BOTTOM_S, 1.0f);
                //back of cube ----------------------------------------------------------------
                block_index += 6;
                buffer[block_index + 0] = getVertex(x + 1, y, z + 1, BACK_S, 0);
                buffer[block_index + 1] = getVertex(x, y, z + 1, BACK_E, 0);
                buffer[block_index + 2] = getVertex(x + 1, y + 1, z + 1, BACK_S, 1.0f);

                buffer[block_index + 3] = getVertex(x, y, z + 1, BACK_E, 0);
                buffer[block_index + 4] = getVertex(x, y + 1, z + 1, BACK_E, 1.0f);
                buffer[block_index + 5] = getVertex(x + 1, y + 1, z + 1, BACK_S, 1.0f);

            }
        }
    }

    memcpy(chunk->vertexMap, buffer, chunk->vertexView.SizeInBytes);
    delete[] buffer;
}
Chunk* CreateChunk(DeviceResources* device, int x_grid_coord, int z_grid_coord, std::vector<int> heightMap)
{
    Chunk* newChunk = new Chunk();

    if (heightMap.size() != 0)
    {
        if (heightMap.size() != BLOCK_COUNT_Z * BLOCK_COUNT_X)
        {
            fprintf(stderr, "Incorrect number of height levels for chunk");
            exit(-1);
        }
        newChunk->heightMap = heightMap;
    }
    else
    {
        newChunk->heightMap = vector<int>(BLOCK_COUNT_Z * BLOCK_COUNT_X, BLOCK_COUNT_Y);
    }

    newChunk->blockGrid = new BlockType[BLOCK_COUNT_X * BLOCK_COUNT_Y * BLOCK_COUNT_Z];
    for (int z = 0; z < BLOCK_COUNT_Z; z++)
    {
        for (int y = 0; y < BLOCK_COUNT_Y; y++)
        {
            for (int x = 0; x < BLOCK_COUNT_X; x++)
            {
                int index = z * BLOCK_COUNT_Y * BLOCK_COUNT_X + y * BLOCK_COUNT_X + x;
                int airLevel = newChunk->heightMap[z * BLOCK_COUNT_X + x];
                if (y < airLevel)
                {
                    newChunk->blockGrid[index] = BlockType::air;
                }
                else
                {
                   newChunk->blockGrid[index] = BlockType::grass;
                }

            }
        }
    }

    device->CreateUploadBuffer(&newChunk->memoryForBlocksIndecies, BLOCK_COUNT_X *
        BLOCK_COUNT_Y * BLOCK_COUNT_Z * sizeof(unsigned int) * INDECIES_PER_CUBE);

    int cbufferSize = ceil(sizeof(ChunkCbuffer) / 256.0f) * 256;
    device->CreateUploadBuffer(&newChunk->cbuffer, cbufferSize);

    D3D12_RANGE readRange{ 0,0 };
    newChunk->memoryForBlocksIndecies->Map(0, &readRange, &newChunk->indexMap);
    newChunk->cbuffer->Map(0, &readRange, &newChunk->cbufferMap);

    newChunk->indexCount = 0;

    newChunk->indexView.BufferLocation = newChunk->memoryForBlocksIndecies->GetGPUVirtualAddress();
    newChunk->indexView.Format = DXGI_FORMAT_R32_UINT;
    newChunk->indexView.SizeInBytes = BLOCK_COUNT_X * BLOCK_COUNT_Y * BLOCK_COUNT_Z * sizeof(unsigned int) * INDECIES_PER_CUBE;
    
    newChunk->cbufferHost.chunkOffsetX = 2 * abs(x_coord_start) * x_grid_coord;
    newChunk->cbufferHost.chunkOffsetZ = 2 * abs(z_coord_start) * z_grid_coord;
    memcpy(newChunk->cbufferMap, &newChunk->cbufferHost, sizeof(ChunkCbuffer));

    if (!newChunk->isVertexBufferInitialized)
    {
        newChunk->vertexView.SizeInBytes = BLOCK_COUNT_X * BLOCK_COUNT_Y * BLOCK_COUNT_Z * VERTECIES_PER_CUBE * sizeof(Vertex);

        device->CreateUploadBuffer(&newChunk->memoryForBlocksVertecies, BLOCK_COUNT_X *
            BLOCK_COUNT_Y * BLOCK_COUNT_Z * sizeof(Vertex) * VERTECIES_PER_CUBE);
        HRESULT hr;
        hr = newChunk->memoryForBlocksVertecies->Map(0, &readRange, &newChunk->vertexMap);

        buildMesh(newChunk);

        newChunk->vertexView.BufferLocation = newChunk->memoryForBlocksVertecies->GetGPUVirtualAddress();
        newChunk->vertexView.StrideInBytes = sizeof(Vertex);

        newChunk->isVertexBufferInitialized = true;
    }

    newChunk->boundingVolume.sphereCenter = XMFLOAT3{ 
        newChunk->cbufferHost.chunkOffsetX, 
        0, 
        newChunk->cbufferHost.chunkOffsetZ 
    };
    newChunk->boundingVolume.sphereRadius = sqrt( pow(x_coord_start, 2) + pow(y_coord_start, 2) + pow(z_coord_start, 2) );
    return newChunk;
}

BlockType GetBlockType(Chunk* chunk, unsigned int x, unsigned int y, unsigned int z)
{
    if (chunk == nullptr) return BlockType::grass;//to show world edges change to air
    return chunk->blockGrid[z * BLOCK_COUNT_Y * BLOCK_COUNT_X + y * BLOCK_COUNT_X + x];
}

void UpdateGpuMemory(Chunk* chunk, Chunk* leftNeighbour, Chunk* rightNeighbour, Chunk* backNeighbour, Chunk* frontNeighbour)
{

    //create index buffer
    unsigned int* cube_indicies = new unsigned int[INDECIES_PER_CUBE * BLOCK_COUNT_X * BLOCK_COUNT_Y * BLOCK_COUNT_Z];
    unsigned int storedBlockIndicies = 0;
    for (int z = 0; z < BLOCK_COUNT_Z; z++)
    {
        for (int y = 0; y < BLOCK_COUNT_Y; y++)
        {
            for (int x = 0; x < BLOCK_COUNT_X; x++)
            {
                if (GetBlockType(chunk, x, y, z) == BlockType::air)
                {
                    continue;
                }

                unsigned int currentBlock = z * BLOCK_COUNT_Y * BLOCK_COUNT_X + y * BLOCK_COUNT_X + x;
                unsigned int vertex_offset = VERTECIES_PER_CUBE * currentBlock;

                unsigned int index_offset;

                if (z > 0 ?
                    GetBlockType(chunk, x, y, z - 1) == BlockType::air : GetBlockType(frontNeighbour, x, y, BLOCK_COUNT_Z -1 ) == BlockType::air )
                {
                    //front face
                    index_offset = storedBlockIndicies;
                    storedBlockIndicies += 6;
                    cube_indicies[index_offset + 0] = vertex_offset + 0;
                    cube_indicies[index_offset + 1] = vertex_offset + 1;
                    cube_indicies[index_offset + 2] = vertex_offset + 2;

                    cube_indicies[index_offset + 3] = vertex_offset + 3;
                    cube_indicies[index_offset + 4] = vertex_offset + 4;
                    cube_indicies[index_offset + 5] = vertex_offset + 5;
                }
                if (x < BLOCK_COUNT_X - 1 ?
                    GetBlockType(chunk, x + 1, y, z) == BlockType::air : GetBlockType(rightNeighbour, 0, y, z) == BlockType::air)
                {
                    //right face 
                    index_offset = storedBlockIndicies;
                    storedBlockIndicies += 6;
                    cube_indicies[index_offset + 0] = vertex_offset + 6;
                    cube_indicies[index_offset + 1] = vertex_offset + 7;
                    cube_indicies[index_offset + 2] = vertex_offset + 8;

                    cube_indicies[index_offset + 3] = vertex_offset + 9;
                    cube_indicies[index_offset + 4] = vertex_offset + 10;
                    cube_indicies[index_offset + 5] = vertex_offset + 11;
                }
                if (x > 0 ?
                    GetBlockType(chunk, x - 1, y, z) == BlockType::air : GetBlockType(leftNeighbour, BLOCK_COUNT_X - 1, y, z) == BlockType::air)
                {
                    //left face
                    index_offset = storedBlockIndicies;
                    storedBlockIndicies += 6;
                    cube_indicies[index_offset + 0] = vertex_offset + 12;
                    cube_indicies[index_offset + 1] = vertex_offset + 13;
                    cube_indicies[index_offset + 2] = vertex_offset + 14;

                    cube_indicies[index_offset + 3] = vertex_offset + 15;
                    cube_indicies[index_offset + 4] = vertex_offset + 16;
                    cube_indicies[index_offset + 5] = vertex_offset + 17;
                }
                if (y > 0 ? GetBlockType(chunk, x, y - 1, z) == BlockType::air : false)
                {
                    //top face     
                    index_offset = storedBlockIndicies;
                    storedBlockIndicies += 6;
                    cube_indicies[index_offset + 0] = vertex_offset + 18;
                    cube_indicies[index_offset + 1] = vertex_offset + 19;
                    cube_indicies[index_offset + 2] = vertex_offset + 20;

                    cube_indicies[index_offset + 3] = vertex_offset + 21;
                    cube_indicies[index_offset + 4] = vertex_offset + 22;
                    cube_indicies[index_offset + 5] = vertex_offset + 23;
                }
                if (y < BLOCK_COUNT_Y - 1 ? GetBlockType(chunk, x, y + 1, z) == BlockType::air : false)
                {
                    //bottom face
                    index_offset = storedBlockIndicies;
                    storedBlockIndicies += 6;
                    cube_indicies[index_offset + 0] = vertex_offset + 24;
                    cube_indicies[index_offset + 1] = vertex_offset + 25;
                    cube_indicies[index_offset + 2] = vertex_offset + 26;

                    cube_indicies[index_offset + 3] = vertex_offset + 27;
                    cube_indicies[index_offset + 4] = vertex_offset + 28;
                    cube_indicies[index_offset + 5] = vertex_offset + 29;
                }
                  
                if (z < BLOCK_COUNT_Z - 1  ?
                    GetBlockType(chunk, x, y, z + 1) == BlockType::air : GetBlockType(backNeighbour, x, y, 0) == BlockType::air)
                {
                    //back face  
                    index_offset = storedBlockIndicies;
                    storedBlockIndicies += 6;
                    cube_indicies[index_offset + 0] = vertex_offset + 30;
                    cube_indicies[index_offset + 1] = vertex_offset + 31;
                    cube_indicies[index_offset + 2] = vertex_offset + 32;

                    cube_indicies[index_offset + 3] = vertex_offset + 33;
                    cube_indicies[index_offset + 4] = vertex_offset + 34;
                    cube_indicies[index_offset + 5] = vertex_offset + 35;
                }

            }
        }
    }
    chunk->indexCount = storedBlockIndicies;
    chunk->indexView.SizeInBytes = storedBlockIndicies * sizeof(unsigned int);
    memcpy(chunk->indexMap, cube_indicies, chunk->indexView.SizeInBytes);
    delete[] cube_indicies;
}
