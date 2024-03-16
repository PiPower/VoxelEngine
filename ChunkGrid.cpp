#include "ChunkGrid.h"
#include <random>

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

struct Vector2D
{
    float x, y;
};

float interpolate(float a0, float a1, float w) {
    return (a1 - a0) * ((w * (w * 6.0 - 15.0) + 10.0) * w * w * w) + a0;
}

Vector2D randomGradient(int ix, int iy)
{
    // No precomputed gradients mean this works for any number of grid coordinates
    const unsigned w = 8 * sizeof(unsigned);
    const unsigned s = w / 2;
    unsigned a = ix, b = iy;
    a *= 3284157443;

    b ^= a << s | a >> w - s;
    b *= 1911520717;

    a ^= b << s | b >> w - s;
    a *= 2048419325;
    float random = a * (3.14159265 / ~(~0u >> 1)); // in [0, 2*Pi]

    // Create the vector from the angle
    Vector2D v;
    v.x = sin(random);
    v.y = cos(random);

    return v;
}

Vector2D* generateNoiseVec(unsigned int x_size, unsigned int y_size)
{
    // Seed with a real random value, if available
    std::random_device r;

    // Choose a random mean between 1 and 6
    std::default_random_engine e1(r());
    std::uniform_real_distribution<float> uniform_dist(-1, 1);
    Vector2D* grid = new Vector2D[x_size * y_size];
    for (int i = 0; i < x_size * y_size; i++)
    {
        grid[i].x = uniform_dist(e1);
        grid[i].y = uniform_dist(e1);
        float norm = sqrt(pow(grid[i].x, 2) + pow(grid[i].y, 2));
        grid[i].x = grid[i].x / norm;
        grid[i].y = grid[i].y / norm;
    }
    return grid;
}

float perlinStep(float grid_x, float grid_z)
{

    int x_0 = floor(grid_x);
    int x_1 = x_0 + 1;
    int z_0 = floor(grid_z);
    int z_1 = z_0 + 1;

    Vector2D gradient = randomGradient(x_0, z_0); //&vectorGrid[z_0 * x_size + x_0];
    Vector2D gradient2 = randomGradient(x_1, z_0);//&vectorGrid[z_0 * x_size + x_1];

    Vector2D gradient3 = randomGradient(x_0, z_1); //&vectorGrid[z_1 * x_size + x_0];
    Vector2D gradient4 = randomGradient(x_1, z_1);//&vectorGrid[z_1 * x_size + x_1];

    float n_0 = (grid_x - x_0) * gradient.x + (grid_z - z_0) * gradient.y;
    float n_1 = (grid_x - x_1) * gradient2.x + (grid_z - z_0) * gradient2.y;
    float interpoland_1 = interpolate(n_0, n_1, grid_x - (float)x_0);

    n_0 = (grid_x - x_0) * gradient3.x + (grid_z - z_1) * gradient3.y;
    n_1 = (grid_x - x_1) * gradient4.x + (grid_z - z_1) * gradient4.y;
    float interpoland_2 = interpolate(n_0, n_1, grid_x - (float)x_0);

    float value = interpolate(interpoland_1, interpoland_2, grid_z - (float)z_0);
    return value;
}
std::vector<int> GetHeightLevelsForChunk(unsigned int x_coord, unsigned int z_coord, 
                                    unsigned int x_size, unsigned int z_size)
{
    std::vector<int> out;

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
                value += perlinStep(grid_x * (freq / BLOCK_COUNT_X), grid_z * (freq/ BLOCK_COUNT_Z)) * amp;

                freq *= 2;
                amp /= 2;
            }

            if (value > 1.0f) value = 1.0f;
            if (value < -1.0f) value = -1.0f;
            out.push_back(BLOCK_COUNT_Y * (value + 1.0f) / 2.0f);
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


