#include "ChunkGrid.h"
#include <random>
#include "Noise.h"
#define THREAD_COUNT 4
SYNCHRONIZATION_BARRIER cpuBarrier;
HANDLE threadGeneratingChunks;
HANDLE threadUpdateChunks;

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

            if (backNeighbour == nullptr || frontNeighbour == nullptr || leftNeighbour == nullptr || rightNeighbour == nullptr)
            {
                continue;
            } 

            UpdateGpuMemory(threadData->device, grid->gridOfChunks[gridIndex], leftNeighbour, rightNeighbour, backNeighbour, frontNeighbour);
            grid->gridOfChunks[gridIndex]->drawable = true;
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
    grid->totalRenderableChunks = (X_halfWidthRender * 2) * (2 * Z_halfWidthRender);
    grid->totalProcessedChunks = ((X_halfWidthRender + 1) * 2) * (2 * (1 + Z_halfWidthRender)) ;

    grid->gridOfChunks = new Chunk*[grid->totalChunks];
    memset(grid->gridOfChunks, 0, sizeof(Chunk*) * grid->totalChunks);

    HANDLE threadHandles[THREAD_COUNT];
    for (int threadId = 0; threadId < THREAD_COUNT; threadId++)
    {
        ThreadData* perThreadData = new ThreadData;
        perThreadData->threadCount = THREAD_COUNT;
        perThreadData->startX = threadId - (int)X_halfWidthRender;
        perThreadData->startZ = -(int)Z_halfWidthRender;
        perThreadData->limitX = (int)X_halfWidthRender;
        perThreadData->limitZ = (int)Z_halfWidthRender;
        perThreadData->X_halfWidth = X_halfWidth;
        perThreadData->Z_halfWidth = Z_halfWidth;
        perThreadData->device = device;
        perThreadData->grid = grid;

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

XMINT2 GetGridCoordsFromRenderingChunkIndex(ChunkGrid* chunkGrid, float centerX, float centerZ, unsigned int index)
{
    int chunkX = floor((centerX - x_coord_start) / (2 * abs(x_coord_start)));
    int chunkZ = floor((centerZ - z_coord_start) / (2 * abs(z_coord_start)));

    int leftCorner = chunkX - (int)chunkGrid->X_halfWidthRender - 1;
    int bottomCorner = chunkZ - (int)chunkGrid->Z_halfWidthRender - 1;

    int rowOffset = (float)index / ((chunkGrid->X_halfWidthRender + 1) * 2 + 1);
    int xOffset = index - rowOffset * ((chunkGrid->X_halfWidthRender + 1) * 2 + 1);

    XMINT2 out;
    out.x = leftCorner + xOffset;
    out.y = bottomCorner + rowOffset;
    return out;
}



std::queue<ChunkGenerationInfo*> threadCreationQueue;
HANDLE producerConsumerCreationMutex = nullptr;
//producer
DWORD _stdcall AdditionalChunkGenerator(void * data)
{
    ChunkGrid* chunkGrid = ((ChunkGenerationStartData*)data)->grid;
    DeviceResources* device = ((ChunkGenerationStartData*)data)->device;
    delete (ChunkGenerationStartData*)data;
    while (true)
    {
        DWORD status = WaitForSingleObject(producerConsumerCreationMutex, INFINITE);
        switch (status)
        {
        case WAIT_ABANDONED:
            OutputDebugString(L"Mutex for chunk generation queue, possible memory corruption \n");
            exit(-1);
        case WAIT_FAILED:
            OutputDebugString(L"Mutex lock error for chunk generation\n");
            exit(GetLastError());
        }

        if (threadCreationQueue.size() == 0)
        {
            ReleaseMutex(producerConsumerCreationMutex);
            continue;
        }
        ChunkGenerationInfo* chunkMetaData = threadCreationQueue.front();
        threadCreationQueue.pop();
        ReleaseMutex(producerConsumerCreationMutex);

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

        if (chunkGrid->gridOfChunks[chunkIndex] == nullptr)
        {
            chunkGrid->gridOfChunks[chunkIndex] = CreateChunk(device, chunkPosX, chunkPosZ, heightLevels);
        }

    }

    return TRUE;
}

std::queue<ChunkGenerationInfo*> threadUpdateQueue;
HANDLE producerConsumerUpdateMutex = nullptr;
DWORD _stdcall AdditionalChunkIndexBufferUpdater(void* data)
{
    ChunkGrid* chunkGrid = ((ChunkGenerationStartData*)data)->grid;
    DeviceResources* device = ((ChunkGenerationStartData*)data)->device;
    delete (ChunkGenerationStartData*)data;
    while (true)
    {
        DWORD status = WaitForSingleObject(producerConsumerUpdateMutex, INFINITE);
        switch (status)
        {
        case WAIT_ABANDONED:
            OutputDebugString(L"Mutex for chunk generation queue, possible memory corruption \n");
            exit(-1);
        case WAIT_FAILED:
            OutputDebugString(L"Mutex lock error for chunk generation\n");
            exit(GetLastError());
        }
        if (threadUpdateQueue.size() == 0)
        {
            ReleaseMutex(producerConsumerUpdateMutex);
            continue;
        }
        ChunkGenerationInfo* chunkMetaData = threadUpdateQueue.front();
        threadUpdateQueue.pop();
        ReleaseMutex(producerConsumerUpdateMutex);

        int chunkPosX = chunkMetaData->posX;
        int chunkPosZ = chunkMetaData->posZ;
        delete chunkMetaData;

        unsigned int chunkIndex = (chunkPosZ + (int)chunkGrid->Z_halfWidth) *
            (chunkGrid->X_halfWidth * 2 + 1) + (chunkPosX + (int)chunkGrid->X_halfWidth);

        if (chunkGrid->gridOfChunks[chunkIndex]->drawable)
        {
            continue;
        }

        //2nd stage build its index buffer to be rendered
        Chunk* leftNeighbour = chunkPosX > -(int)chunkGrid->X_halfWidth ? chunkGrid->gridOfChunks[chunkIndex - 1] : nullptr;
        Chunk* rightNeighbour = chunkPosX < (int)chunkGrid->X_halfWidth ? chunkGrid->gridOfChunks[chunkIndex + 1] : nullptr;

        Chunk* frontNeighbour = chunkPosZ > -(int)chunkGrid->Z_halfWidth ?
            chunkGrid->gridOfChunks[chunkIndex - (chunkGrid->X_halfWidth * 2 + 1)] : nullptr;

        Chunk* backNeighbour = chunkPosZ < (int)chunkGrid->Z_halfWidth ?
            chunkGrid->gridOfChunks[chunkIndex + (chunkGrid->X_halfWidth * 2 + 1)] : nullptr;


        if (backNeighbour == nullptr || frontNeighbour == nullptr || leftNeighbour == nullptr || rightNeighbour == nullptr)
        {
            continue;
        }
        UpdateGpuMemory(device, chunkGrid->gridOfChunks[chunkIndex], leftNeighbour, rightNeighbour, backNeighbour, frontNeighbour);
        chunkGrid->gridOfChunks[chunkIndex]->drawable = true;
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

        producerConsumerCreationMutex = CreateMutex(NULL, FALSE, NULL);
        threadGeneratingChunks = CreateThread(NULL, 0, AdditionalChunkGenerator, startData, 0, NULL);
        WaitForSingleObject(threadGeneratingChunks, 0.02);

    }

    ChunkGenerationInfo* chunkData = new ChunkGenerationInfo();
    chunkData->posX = chunkPosX;
    chunkData->posZ = chunkPosZ;


    DWORD status = WaitForSingleObject(producerConsumerCreationMutex, INFINITE);
    switch (status)
    {
    case WAIT_ABANDONED:
        OutputDebugString(L"Mutex for chunk generation queue, possible memory corruption \n");
        exit(-1);
    case WAIT_FAILED:
        OutputDebugString(L"Mutex lock error for chunk generation\n");
        exit(GetLastError());
    }
    threadCreationQueue.push(chunkData);
    ReleaseMutex(producerConsumerCreationMutex);
}




XMINT2 GetCameraPosInGrid(float x, float z)
{
    int centerX = floor((x - x_coord_start) / (2 * abs(x_coord_start)));
    int centerZ = floor((z - z_coord_start) / (2 * abs(z_coord_start)));

    float yolo = (x - x_coord_start) / (2 * abs(x_coord_start));

    return XMINT2(centerX, centerZ);
}

bool IsInBorder(ChunkGrid* chunkGrid, XMINT2 camCenter, XMINT2 chunkPos)
{

    if (chunkPos.x == (camCenter.x + chunkGrid->X_halfWidthRender + 1) || chunkPos.x == (camCenter.x - chunkGrid->X_halfWidthRender - 1))
    {
        return true;
    }

    if (chunkPos.y == (camCenter.y + chunkGrid->Z_halfWidthRender + 1) || chunkPos.y == (camCenter.y - chunkGrid->Z_halfWidthRender - 1))
    {
        return true;
    }

    return false;
}

void UpdateChunkIndexBuffer(DeviceResources* device, ChunkGrid* chunkGrid, int chunkPosX, int chunkPosZ)
{

    if (threadUpdateChunks == nullptr)
    {
        ChunkGenerationStartData* startData = new ChunkGenerationStartData();
        startData->device = device;
        startData->grid = chunkGrid;

        producerConsumerUpdateMutex = CreateMutex(NULL, FALSE, NULL);
        threadUpdateChunks = CreateThread(NULL, 0, AdditionalChunkIndexBufferUpdater, startData, 0, NULL);
        WaitForSingleObject(threadUpdateChunks, 0.02);

    }

    ChunkGenerationInfo* chunkData = new ChunkGenerationInfo();
    chunkData->posX = chunkPosX;
    chunkData->posZ = chunkPosZ;


    DWORD status = WaitForSingleObject(producerConsumerUpdateMutex, INFINITE);
    switch (status)
    {
    case WAIT_ABANDONED:
        OutputDebugString(L"Mutex for chunk memory update queue, possible memory corruption \n");
        exit(-1);
    case WAIT_FAILED:
        OutputDebugString(L"Mutex lock error for chunk memory update\n");
        exit(GetLastError());
    }

    threadUpdateQueue.push(chunkData);
    ReleaseMutex(producerConsumerUpdateMutex);
}

bool IsInBorder(ChunkGrid* chunkGrid, unsigned int index)
{

    int rowOffset = (float)index / (chunkGrid->X_halfWidthRender * 2 + 1);
    int xOffset = index - rowOffset * (chunkGrid->X_halfWidthRender * 2 + 1);

    if ( (rowOffset == 0 || rowOffset == (chunkGrid->Z_halfWidthRender * 2 + 1))
        &&
        (xOffset == 0 || xOffset == (chunkGrid->X_halfWidthRender * 2 + 1))  )
    {
        return true;
    }
    return false;
}


