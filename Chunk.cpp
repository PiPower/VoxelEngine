#include "Chunk.h"

Chunk* CreateChunk(DeviceResources* device)
{
    Chunk* newChunk = new Chunk();
    newChunk->blockGrid = new BlockType[BLOCK_COUNT_X * BLOCK_COUNT_Y * BLOCK_COUNT_Z];
    newChunk->memoryForBlocksVertecies = device->CreateUploadBuffer(BLOCK_COUNT_X *
                                    BLOCK_COUNT_Y * BLOCK_COUNT_Z * sizeof(Vertex) );
    newChunk->memoryForBlocksIndecies = device->CreateUploadBuffer(BLOCK_COUNT_X *
                                BLOCK_COUNT_Y * BLOCK_COUNT_Z * sizeof(unsigned int) );

    for (int i = 0; i < BLOCK_COUNT_X * BLOCK_COUNT_Y * BLOCK_COUNT_Z; i++)
    {
        newChunk->blockGrid[i] = BlockType::none;
    }
    return newChunk;
}
