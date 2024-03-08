#include "Chunk.h"

Chunk* CreateChunk(DeviceResources* device)
{
    Chunk* newChunk = new Chunk();

    newChunk->blockGrid = new BlockType[BLOCK_COUNT_X * BLOCK_COUNT_Y * BLOCK_COUNT_Z];
    device->CreateUploadBuffer(&newChunk->memoryForBlocksVertecies, BLOCK_COUNT_X *
                                    BLOCK_COUNT_Y * BLOCK_COUNT_Z * sizeof(Vertex) * VERTEX_PER_CUBE);
    device->CreateUploadBuffer(&newChunk->memoryForBlocksIndecies, BLOCK_COUNT_X *
                                BLOCK_COUNT_Y * BLOCK_COUNT_Z * sizeof(unsigned int) * INDEX_PER_CUBE);

    D3D12_RANGE readRange{ 0,0 };
    newChunk->memoryForBlocksIndecies->Map(0, &readRange, &newChunk->indexMap);
    newChunk->memoryForBlocksVertecies->Map(0, &readRange, &newChunk->vertexMap);

    for (int i = 0; i < BLOCK_COUNT_X * BLOCK_COUNT_Y * BLOCK_COUNT_Z; i++)
    {
        newChunk->blockGrid[i] = BlockType::grass;
    }

    newChunk->indexCount = INDEX_PER_CUBE * BLOCK_COUNT_X * BLOCK_COUNT_Y * BLOCK_COUNT_Z;

    newChunk->vertexView.BufferLocation = newChunk->memoryForBlocksVertecies->GetGPUVirtualAddress();
    newChunk->vertexView.SizeInBytes = BLOCK_COUNT_X * BLOCK_COUNT_Y * BLOCK_COUNT_Z * VERTEX_PER_CUBE * sizeof(Vertex);
    newChunk->vertexView.StrideInBytes = sizeof(Vertex);

    newChunk->indexView.BufferLocation = newChunk->memoryForBlocksIndecies->GetGPUVirtualAddress();
    newChunk->indexView.Format = DXGI_FORMAT_R32_UINT;
    newChunk->indexView.SizeInBytes = BLOCK_COUNT_X * BLOCK_COUNT_Y * BLOCK_COUNT_Z * sizeof(unsigned int) * INDEX_PER_CUBE;

    return newChunk;
}

void UpdateGpuMemory(Chunk* chunk)
{
    Vertex buffer[BLOCK_COUNT_X * BLOCK_COUNT_Y * BLOCK_COUNT_Z * VERTEX_PER_CUBE];
    //create voxel mesh
    for (int z = 0; z < BLOCK_COUNT_Z; z++)
    {
        for (int y = 0; y < BLOCK_COUNT_Y; y++)
        {
            for (int x = 0; x < BLOCK_COUNT_X; x++)
            {
                Vertex vertex = {};
                unsigned int block_index = VERTEX_PER_CUBE * (z * BLOCK_COUNT_Y * BLOCK_COUNT_X + y * BLOCK_COUNT_X + x);

                //front of cube ----------------------------------------------------------
                vertex.x = -1.0f + 2.0f * x / BLOCK_COUNT_X;
                vertex.y = 1.0f - 2.0f * y / BLOCK_COUNT_Y;
                vertex.z = -1.0f + 2.0f * z / BLOCK_COUNT_Z;
                buffer[block_index] = vertex;

                vertex.x = -1.0f + 2.0f * (x+1) / BLOCK_COUNT_X;
                vertex.y = 1.0f - 2.0f * y / BLOCK_COUNT_Y;
                vertex.z = -1.0f + 2.0f * z / BLOCK_COUNT_Z;
                buffer[block_index + 1] = vertex;

                vertex.x = -1.0f + 2.0f * x / BLOCK_COUNT_X;
                vertex.y = 1.0f - 2.0f * (y+1) / BLOCK_COUNT_Y;
                vertex.z = -1.0f + 2.0f * z / BLOCK_COUNT_Z;
                buffer[block_index + 2] = vertex;

                vertex.x = -1.0f + 2.0f * (x + 1) / BLOCK_COUNT_X;
                vertex.y = 1.0f - 2.0f * (y + 1)/ BLOCK_COUNT_Y;
                vertex.z = -1.0f + 2.0f * z / BLOCK_COUNT_Z;
                buffer[block_index + 3] = vertex;
                //back of cube ----------------------------------------------------------
                vertex.x = -1.0f + 2.0f * x / BLOCK_COUNT_X;
                vertex.y = 1.0f - 2.0f * y / BLOCK_COUNT_Y;
                vertex.z = -1.0f + 2.0f * (z+1) / BLOCK_COUNT_Z;
                buffer[block_index + 4] = vertex;

                vertex.x = -1.0f + 2.0f * (x + 1) / BLOCK_COUNT_X;
                vertex.y = 1.0f - 2.0f * y / BLOCK_COUNT_Y;
                vertex.z = -1.0f + 2.0f * (z + 1) / BLOCK_COUNT_Z;
                buffer[block_index + 5] = vertex;

                vertex.x = -1.0f + 2.0f * x / BLOCK_COUNT_X;
                vertex.y = 1.0f - 2.0f * (y + 1) / BLOCK_COUNT_Y;
                vertex.z = -1.0f + 2.0f * (z + 1) / BLOCK_COUNT_Z;
                buffer[block_index + 6] = vertex;

                vertex.x = -1.0f + 2.0f * (x + 1) / BLOCK_COUNT_X;
                vertex.y = 1.0f - 2.0f * (y + 1) / BLOCK_COUNT_Y;
                vertex.z = -1.0f + 2.0f * (z + 1) / BLOCK_COUNT_Z;
                buffer[block_index + 7] = vertex;


            }
        }
    }
    memcpy(chunk->vertexMap, buffer, chunk->vertexView.SizeInBytes);
    //create index buffer
    unsigned int cube_indicies[INDEX_PER_CUBE * BLOCK_COUNT_X * BLOCK_COUNT_Y * BLOCK_COUNT_Z] = {};
    for (int z = 0; z < BLOCK_COUNT_Z; z++)
    {
        for (int y = 0; y < BLOCK_COUNT_Y; y++)
        {
            for (int x = 0; x < BLOCK_COUNT_X; x++)
            {
                unsigned int vertex_offset = VERTEX_PER_CUBE * (z * BLOCK_COUNT_Z * BLOCK_COUNT_Y + y * BLOCK_COUNT_X + x);
                unsigned int index_offset = INDEX_PER_CUBE * (z * BLOCK_COUNT_Z * BLOCK_COUNT_Y + y * BLOCK_COUNT_X + x);

                cube_indicies[index_offset + 0] = vertex_offset;
                cube_indicies[index_offset + 1] = vertex_offset + 1;
                cube_indicies[index_offset + 2] = vertex_offset + 2;

                cube_indicies[index_offset + 3] = vertex_offset + 1;
                cube_indicies[index_offset + 4] = vertex_offset + 3;
                cube_indicies[index_offset + 5] = vertex_offset + 2;
            }
        }
    }
    memcpy(chunk->indexMap, cube_indicies, chunk->indexView.SizeInBytes);
}
