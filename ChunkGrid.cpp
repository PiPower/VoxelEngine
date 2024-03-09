#include "ChunkGrid.h"

ChunkGrid* CreateChunkGrid(DeviceResources* device, unsigned int X_halfWidth, unsigned int Z_halfWidth)
{
    ChunkGrid* grid = new ChunkGrid();
    grid->X_halfWidth = X_halfWidth;
    grid->Z_halfWidth = Z_halfWidth;
    grid->totalChunks = (X_halfWidth * 2 + 1) * (2 * Z_halfWidth + 1);
    grid->gridOfChunks = new Chunk*[grid->totalChunks];

    for(int z = -(int)Z_halfWidth ; z <= (int)Z_halfWidth; z++)
    {
        for (int x = -(int)X_halfWidth; x <= (int)X_halfWidth; x++)
        {
            int gridIndex = (z + (int)Z_halfWidth) * (X_halfWidth * 2 + 1) + (x + (int)X_halfWidth);
            grid->gridOfChunks[gridIndex] = CreateChunk(device, x, z);
            UpdateGpuMemory(grid->gridOfChunks[gridIndex]);
        }
    }
    return grid;
}
