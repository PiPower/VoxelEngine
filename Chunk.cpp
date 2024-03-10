#include "Chunk.h"
#include <random>

constexpr float x_coord_start = -1.0f;
constexpr float y_coord_start = (float)BLOCK_COUNT_Y / BLOCK_COUNT_X;
constexpr float z_coord_start = -(float)BLOCK_COUNT_Z / BLOCK_COUNT_X;

bool Chunk::isVertexBufferInitialized = false;
ComPtr<ID3D12Resource> Chunk::memoryForBlocksVertecies;
D3D12_VERTEX_BUFFER_VIEW Chunk::vertexView = {};
DevicePointer Chunk::vertexMap = nullptr;


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
                //front of cube ----------------------------------------------------------
                vertex.x = x_coord_start + 2.0f * abs(x_coord_start) * x / BLOCK_COUNT_X;
                vertex.y = y_coord_start - 2.0f * abs(y_coord_start) * y / BLOCK_COUNT_Y;
                vertex.z = z_coord_start + 2.0f * abs(z_coord_start) * z / BLOCK_COUNT_Z;
                buffer[block_index] = vertex;

                vertex.x = x_coord_start + 2.0f * abs(x_coord_start) * (x + 1) / BLOCK_COUNT_X;
                vertex.y = y_coord_start - 2.0f * abs(y_coord_start) * y / BLOCK_COUNT_Y;
                vertex.z = z_coord_start + 2.0f * abs(z_coord_start) * z / BLOCK_COUNT_Z;
                buffer[block_index + 1] = vertex;

                vertex.x = x_coord_start + 2.0f * abs(x_coord_start) * x / BLOCK_COUNT_X;
                vertex.y = y_coord_start - 2.0f * abs(y_coord_start) * (y + 1) / BLOCK_COUNT_Y;
                vertex.z = z_coord_start + 2.0f * abs(z_coord_start) * z / BLOCK_COUNT_Z;
                buffer[block_index + 2] = vertex;

                vertex.x = x_coord_start + 2.0f * abs(x_coord_start) * (x + 1) / BLOCK_COUNT_X;
                vertex.y = y_coord_start - 2.0f * abs(y_coord_start) * (y + 1) / BLOCK_COUNT_Y;
                vertex.z = z_coord_start + 2.0f * abs(z_coord_start) * z / BLOCK_COUNT_Z;
                buffer[block_index + 3] = vertex;
                //back of cube ----------------------------------------------------------
                vertex.x = x_coord_start + 2.0f * abs(x_coord_start) * x / BLOCK_COUNT_X;
                vertex.y = y_coord_start - 2.0f * abs(y_coord_start) * y / BLOCK_COUNT_Y;
                vertex.z = z_coord_start + 2.0f * abs(z_coord_start) * (z + 1) / BLOCK_COUNT_Z;
                buffer[block_index + 4] = vertex;

                vertex.x = x_coord_start + 2.0f * abs(x_coord_start) * (x + 1) / BLOCK_COUNT_X;
                vertex.y = y_coord_start - 2.0f * abs(y_coord_start) * y / BLOCK_COUNT_Y;
                vertex.z = z_coord_start + 2.0f * abs(z_coord_start) * (z + 1) / BLOCK_COUNT_Z;
                buffer[block_index + 5] = vertex;

                vertex.x = x_coord_start + 2.0f * abs(x_coord_start) * x / BLOCK_COUNT_X;
                vertex.y = y_coord_start - 2.0f * abs(y_coord_start) * (y + 1) / BLOCK_COUNT_Y;
                vertex.z = z_coord_start + 2.0f * abs(z_coord_start) * (z + 1) / BLOCK_COUNT_Z;
                buffer[block_index + 6] = vertex;

                vertex.x = x_coord_start + 2.0f * abs(x_coord_start) * (x + 1) / BLOCK_COUNT_X;
                vertex.y = y_coord_start - 2.0f * abs(y_coord_start) * (y + 1) / BLOCK_COUNT_Y;
                vertex.z = z_coord_start + 2.0f * abs(z_coord_start) * (z + 1) / BLOCK_COUNT_Z;
                buffer[block_index + 7] = vertex;
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
        newChunk->heightMap = vector(BLOCK_COUNT_Z * BLOCK_COUNT_X, BLOCK_COUNT_Y);
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
                newChunk->blockGrid[index] = y < airLevel ? BlockType::grass : BlockType::air;
                
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

                bool xAxisShown = (x < BLOCK_COUNT_X - 1 ?
                    GetBlockType(chunk, x + 1, y, z) == BlockType::air : GetBlockType(rightNeighbour, 0, y, z) == BlockType::air)
                    || (x > 0 ?
                        GetBlockType(chunk, x - 1, y, z) == BlockType::air : GetBlockType(leftNeighbour, BLOCK_COUNT_X - 1, y, z) == BlockType::air);

                bool yAxisShown = (y < BLOCK_COUNT_Y - 1 ? GetBlockType(chunk, x, y + 1, z) == BlockType::air : false)
                    || (y > 0 ? GetBlockType(chunk, x, y - 1, z) == BlockType::air : false);

                bool zAxisShown = (z < BLOCK_COUNT_Z - 1 ?
                    GetBlockType(chunk, x, y, z + 1) == BlockType::air : GetBlockType(frontNeighbour, x, y, 0) == BlockType::air)
                    || (z > 0 ?
                        GetBlockType(chunk, x, y, z - 1) == BlockType::air : GetBlockType(backNeighbour, x, y, BLOCK_COUNT_Z - 1) == BlockType::air);

                if ((!(xAxisShown || yAxisShown || zAxisShown)))
                {
                    continue;
                }

                unsigned int currentBlock = z * BLOCK_COUNT_Y * BLOCK_COUNT_X + y * BLOCK_COUNT_X + x;
                unsigned int vertex_offset = VERTECIES_PER_CUBE * currentBlock;

                unsigned int index_offset = INDECIES_PER_CUBE * storedBlockIndicies;
                storedBlockIndicies++;
                //front face
                cube_indicies[index_offset + 0] = vertex_offset + 0;
                cube_indicies[index_offset + 1] = vertex_offset + 1;
                cube_indicies[index_offset + 2] = vertex_offset + 2;

                cube_indicies[index_offset + 3] = vertex_offset + 1;
                cube_indicies[index_offset + 4] = vertex_offset + 3;
                cube_indicies[index_offset + 5] = vertex_offset + 2;

                //right face 
                cube_indicies[index_offset + 6] = vertex_offset + 1;
                cube_indicies[index_offset + 7] = vertex_offset + 5;
                cube_indicies[index_offset + 8] = vertex_offset + 3;

                cube_indicies[index_offset + 9] = vertex_offset + 5;
                cube_indicies[index_offset + 10] = vertex_offset + 7;
                cube_indicies[index_offset + 11] = vertex_offset + 3;

                //left face
                cube_indicies[index_offset + 12] = vertex_offset + 4;
                cube_indicies[index_offset + 13] = vertex_offset + 0;
                cube_indicies[index_offset + 14] = vertex_offset + 6;

                cube_indicies[index_offset + 15] = vertex_offset + 0;
                cube_indicies[index_offset + 16] = vertex_offset + 2;
                cube_indicies[index_offset + 17] = vertex_offset + 6;

                //top face
                cube_indicies[index_offset + 18] = vertex_offset + 0;
                cube_indicies[index_offset + 19] = vertex_offset + 4;
                cube_indicies[index_offset + 20] = vertex_offset + 5;

                cube_indicies[index_offset + 21] = vertex_offset + 0;
                cube_indicies[index_offset + 22] = vertex_offset + 5;
                cube_indicies[index_offset + 23] = vertex_offset + 1;

                //bottom face
                cube_indicies[index_offset + 24] = vertex_offset + 2;
                cube_indicies[index_offset + 25] = vertex_offset + 7;
                cube_indicies[index_offset + 26] = vertex_offset + 6;

                cube_indicies[index_offset + 27] = vertex_offset + 2;
                cube_indicies[index_offset + 28] = vertex_offset + 3;
                cube_indicies[index_offset + 29] = vertex_offset + 7;

                //back face
                cube_indicies[index_offset + 30] = vertex_offset + 6;
                cube_indicies[index_offset + 31] = vertex_offset + 5;
                cube_indicies[index_offset + 32] = vertex_offset + 4;

                cube_indicies[index_offset + 33] = vertex_offset + 7;
                cube_indicies[index_offset + 34] = vertex_offset + 5;
                cube_indicies[index_offset + 35] = vertex_offset + 6;
            }
        }
    }
    chunk->indexCount = INDECIES_PER_CUBE * storedBlockIndicies;
    chunk->indexView.SizeInBytes = INDECIES_PER_CUBE * storedBlockIndicies * sizeof(unsigned int);
    memcpy(chunk->indexMap, cube_indicies, chunk->indexView.SizeInBytes);
    delete[] cube_indicies;
}
