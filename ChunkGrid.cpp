#include "ChunkGrid.h"
#include <random>
#include "Noise.h"
#define THREAD_COUNT 4
SYNCHRONIZATION_BARRIER cpuBarrier;
HANDLE threadGeneratingChunks;

struct ThreadData
{
    unsigned int threadCount;
    int startX;
    int startZ;
    int limitX;
    int limitZ;
    unsigned int X_halfWidth;
    unsigned int Z_halfWidth;
    DeviceResources* device;
    ChunkGrid* grid;
};

struct ChunkGenerationStartData
{
    ChunkGrid* grid;
    DeviceResources* device;
};
struct ChunkGenerationInfo
{
    int posX;
    int posZ;
};

float clip(float x, float min, float max)
{
    if (x < min) { return min; }
    if (x > max) { return max; }
    return x;
}


int clip(int x, int min, int max)
{
    if (x < min) { return min; }
    if (x > max) { return max; }
    return x;
}

std::vector<int> GetHeightLevelsForChunk(unsigned int x_coord, unsigned int z_coord, 
                                    unsigned int x_size, unsigned int z_size)
{
    std::vector<int> out;

    static std::random_device r;
    static std::default_random_engine e1(r());
    static std::uniform_int_distribution<int> dist(40, BLOCK_COUNT_Y);

    int scal = dist(e1);
    for (int z = 0; z < BLOCK_COUNT_Z; z++)
    {
        for (int x = 0; x < BLOCK_COUNT_X; x++)
        {
            float grid_x = (x_coord * BLOCK_COUNT_X + (float)x) / BLOCK_COUNT_X;
            float grid_z = (z_coord * BLOCK_COUNT_Z + (float)z) / BLOCK_COUNT_Z;
            float value = 0;

            float freq = 1;
            float amp = 1;
            for (int i = 0; i < 12; i++)
            {
                float x_in = grid_x * (freq / BLOCK_COUNT_X);
                float z_in = grid_z * (freq / BLOCK_COUNT_Z);
                value += gnoise(x_in, z_in, x_in + z_in) * amp;
                value += gnoise(z_in, x_in +z_in, x_in) * amp;
                freq *= 2;
                amp *= 0.5;
            }
            value = clip(value, -1.0f, 1.0f);

            int target = BLOCK_COUNT_Y * (value + 1.0f) / 2.0f;

            out.push_back(clip(target, 1, BLOCK_COUNT_Y - 1) );
        }
    }
    return out;
}

DWORD _stdcall ThreadCreateChunks(void* threadDataVoid)
{

    ThreadData* threadData = (ThreadData * )threadDataVoid;
    ChunkGrid* grid = threadData->grid;
    unsigned int X_halfWidth = threadData->X_halfWidth;
    unsigned int Z_halfWidth = threadData->Z_halfWidth;

    for (int z = threadData->startZ; z <= threadData->limitZ; z++)
    {
        for (int x = threadData->startX; x <= threadData->limitX; x += threadData->threadCount)
        {
            int gridIndex = (z + (int)threadData->Z_halfWidth) * 
                            (threadData->X_halfWidth * 2 + 1) + (x + (int)threadData->X_halfWidth);

            std::vector<int> heightLevels = GetHeightLevelsForChunk(x + (int)threadData->X_halfWidth,
                z + (int)threadData->Z_halfWidth, threadData->X_halfWidth * 2 + 1, 2 * threadData->Z_halfWidth + 1);

            threadData->grid->gridOfChunks[gridIndex] = CreateChunk(threadData->device, x, z, heightLevels);
        }
    }

    EnterSynchronizationBarrier(&cpuBarrier, SYNCHRONIZATION_BARRIER_FLAGS_SPIN_ONLY);


    for (int z = threadData->startZ; z <= threadData->limitZ; z++)
    {
        for (int x = threadData->startX; x <= threadData->limitX; x += threadData->threadCount)
        {
            int gridIndex = (z + (int)threadData->Z_halfWidth) * (X_halfWidth * 2 + 1) + (x + (int)X_halfWidth);
            Chunk* leftNeighbour = x > threadData->startX ? grid->gridOfChunks[gridIndex - 1] : nullptr;
            Chunk* rightNeighbour = x < threadData->limitX ? grid->gridOfChunks[gridIndex + 1] : nullptr;

            Chunk* frontNeighbour = z > threadData->startZ ? grid->gridOfChunks[gridIndex - (X_halfWidth * 2 + 1)] : nullptr;
            Chunk* backNeighbour = z < threadData->limitZ ? grid->gridOfChunks[gridIndex + (X_halfWidth * 2 + 1)] : nullptr;

            UpdateGpuMemory(threadData->device, grid->gridOfChunks[gridIndex], leftNeighbour, rightNeighbour, backNeighbour, frontNeighbour);
        }
    }

    delete threadData;
    return TRUE;
}


//blocks are stored in form back-front, left-right
ChunkGrid* CreateChunkGrid(DeviceResources* device, 
    unsigned int X_halfWidth, unsigned int Z_halfWidth, unsigned int X_halfWidthRender, unsigned int Z_halfWidthRender)
{
    InitMultithreadingResources();
    InitializeSynchronizationBarrier(&cpuBarrier, THREAD_COUNT, 2000);
    ChunkGrid* grid = new ChunkGrid();
    grid->X_halfWidth = X_halfWidth;
    grid->Z_halfWidth = Z_halfWidth;
    grid->totalChunks = (X_halfWidth * 2 + 1) * (2 * Z_halfWidth + 1);
    grid->X_halfWidthRender = X_halfWidthRender;
    grid->Z_halfWidthRender = Z_halfWidthRender;
    grid->totalRenderableChunks = (X_halfWidthRender * 2 + 1) * (2 * Z_halfWidthRender + 1);

    grid->gridOfChunks = new Chunk*[grid->totalChunks];
    memset(grid->gridOfChunks, 0, sizeof(Chunk*) * grid->totalChunks);


    HANDLE threadHandles[THREAD_COUNT];
    for (int threadId = 0; threadId < THREAD_COUNT; threadId++)
    {
        ThreadData* perThreadData = new ThreadData;
        perThreadData->threadCount = THREAD_COUNT;
        perThreadData->startX = threadId - (int)X_halfWidthRender;
        perThreadData->startZ = -(int)Z_halfWidthRender;
        perThreadData->limitX = (int)Z_halfWidthRender;
        perThreadData->limitZ = (int)X_halfWidthRender;
        perThreadData->X_halfWidth = X_halfWidth;
        perThreadData->Z_halfWidth = Z_halfWidth;
        perThreadData->device = device;
        perThreadData->grid = grid;
        perThreadData->device = device;

        threadHandles[threadId] = CreateThread(NULL, 0, ThreadCreateChunks, perThreadData, 0, NULL);
    }

    WaitForMultipleObjects(THREAD_COUNT, threadHandles, true, INFINITE);

    return grid;
}

Chunk* GetNthRenderableChunkFromCameraPos(ChunkGrid* chunkGrid, float posX, float posZ, unsigned int index)
{
    XMINT2 chunkPos = GetGridCoordsFromRenderingChunkIndex(chunkGrid, posX, posZ, index);

    if (abs(chunkPos.x) > chunkGrid->X_halfWidth || abs(chunkPos.y) > chunkGrid->Z_halfWidth)
    {
        return reinterpret_cast<Chunk*>( 0xFFFFFFFFFFFFFFFF);
    }

    unsigned int chunkIndex = (chunkPos.y + (int)chunkGrid->Z_halfWidth) *
                (chunkGrid->X_halfWidth * 2 + 1) + (chunkPos.x + (int)chunkGrid->X_halfWidth);

    return chunkGrid->gridOfChunks[chunkIndex];
}

XMINT2 GetGridCoordsFromRenderingChunkIndex(ChunkGrid* chunkGrid, float posX, float posZ, unsigned int index)
{
    int centerX = floor((posX - x_coord_start) / (2 * abs(x_coord_start)));
    int centerZ = floor((posZ - z_coord_start) / (2 * abs(z_coord_start)));

    int leftCorner = centerX - (int)chunkGrid->X_halfWidthRender;
    int topCorner = centerZ - (int)chunkGrid->Z_halfWidthRender;

    int rowOffset = (float)index / (chunkGrid->X_halfWidthRender * 2 + 1);
    int xOffset = index - rowOffset * (chunkGrid->X_halfWidthRender * 2 + 1);
    XMINT2 out;
    out.x = leftCorner + xOffset;
    out.y = topCorner + rowOffset;
    return out;
}



std::queue<ChunkGenerationInfo*> threadQueue;
HANDLE producerConsumerMutex = nullptr;
//producer
DWORD _stdcall AdditionalChunkGenerator(void * data)
{
    ChunkGrid* chunkGrid = ((ChunkGenerationStartData*)data)->grid;
    DeviceResources* device = ((ChunkGenerationStartData*)data)->device;
    delete (ChunkGenerationStartData*)data;
    while (true)
    {
        DWORD status = WaitForSingleObject(producerConsumerMutex, INFINITE);
        switch (status)
        {
        case WAIT_ABANDONED:
            OutputDebugString(L"Mutex for chunk generation queue, possible memory corruption \n");
            exit(-1);
        case WAIT_FAILED:
            OutputDebugString(L"Mutex lock error for chunk generation\n");
            exit(GetLastError());
        }
        if (threadQueue.size() == 0)
        {
            ReleaseMutex(producerConsumerMutex);
            continue;
        }
        ChunkGenerationInfo* chunkMetaData = threadQueue.front();
        threadQueue.pop();
        ReleaseMutex(producerConsumerMutex);

        int chunkPosX = chunkMetaData->posX;
        int chunkPosZ = chunkMetaData->posZ;
        delete chunkMetaData;

        unsigned int chunkIndex = (chunkPosZ + (int)chunkGrid->Z_halfWidth) *
            (chunkGrid->X_halfWidth * 2 + 1) + (chunkPosX + (int)chunkGrid->X_halfWidth);

        if (chunkGrid->gridOfChunks[chunkIndex] != nullptr)
        {
            continue;
        }

        std::vector<int> heightLevels = GetHeightLevelsForChunk(chunkPosX + (int)chunkGrid->X_halfWidth,
            chunkPosZ + (int)chunkGrid->Z_halfWidth, chunkGrid->X_halfWidth * 2 + 1, 2 * chunkGrid->Z_halfWidth + 1);
        chunkGrid->gridOfChunks[chunkIndex] = CreateChunk(device, chunkPosX, chunkPosZ, heightLevels);

        Chunk* leftNeighbour = chunkPosX > -(int)chunkGrid->X_halfWidth ? chunkGrid->gridOfChunks[chunkIndex - 1] : nullptr;
        Chunk* rightNeighbour = chunkPosX < chunkGrid->X_halfWidth ? chunkGrid->gridOfChunks[chunkIndex + 1] : nullptr;

        Chunk* frontNeighbour = chunkPosZ > -(int)chunkGrid->Z_halfWidth ?
            chunkGrid->gridOfChunks[chunkIndex - (chunkGrid->X_halfWidth * 2 + 1)] : nullptr;

        Chunk* backNeighbour = chunkPosZ < chunkGrid->Z_halfWidth ?
            chunkGrid->gridOfChunks[chunkIndex + (chunkGrid->X_halfWidth * 2 + 1)] : nullptr;

        UpdateGpuMemory(device, chunkGrid->gridOfChunks[chunkIndex], leftNeighbour, rightNeighbour, backNeighbour, frontNeighbour);

    }

    return TRUE;
}

void GenerateChunk(DeviceResources* device,ChunkGrid* chunkGrid, int chunkPosX, int chunkPosZ)
{

    if (threadGeneratingChunks == nullptr)
    {
        ChunkGenerationStartData*  startData = new ChunkGenerationStartData();
        startData->device = device;
        startData->grid = chunkGrid;

        producerConsumerMutex = CreateMutex(NULL, FALSE, NULL);
        threadGeneratingChunks = CreateThread(NULL, 0, AdditionalChunkGenerator, startData, 0, NULL);
        WaitForSingleObject(threadGeneratingChunks, 0);

    }

    ChunkGenerationInfo* chunkData = new ChunkGenerationInfo();
    chunkData->posX = chunkPosX;
    chunkData->posZ = chunkPosZ;

    DWORD status = WaitForSingleObject(producerConsumerMutex, INFINITE);
    switch (status)
    {
    case WAIT_ABANDONED:
        OutputDebugString(L"Mutex for chunk generation queue, possible memory corruption \n");
        exit(-1);
    case WAIT_FAILED:
        OutputDebugString(L"Mutex lock error for chunk generation\n");
        exit(GetLastError());
    }
    threadQueue.push(chunkData);
    ReleaseMutex(producerConsumerMutex);
}

XMINT2 GetCameraPosInGrid(float x, float z)
{
    int centerX = floor((x - x_coord_start) / (2 * abs(x_coord_start)));
    int centerZ = floor((z - z_coord_start) / (2 * abs(z_coord_start)));
    return XMINT2(centerX, centerZ);
}


