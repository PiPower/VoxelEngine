#include "ChunkGrid.h"
#include <random>
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
                                    unsigned int x_size, unsigned int z_size, Vector2D* vectorGrid)
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


//blocks are stored in form back-front, left-right
ChunkGrid* CreateChunkGrid(DeviceResources* device, unsigned int X_halfWidth, unsigned int Z_halfWidth)
{
    ChunkGrid* grid = new ChunkGrid();
    grid->X_halfWidth = X_halfWidth;
    grid->Z_halfWidth = Z_halfWidth;
    grid->totalChunks = (X_halfWidth * 2 + 1) * (2 * Z_halfWidth + 1);
    grid->gridOfChunks = new Chunk*[grid->totalChunks];
    Vector2D* gridOfNoiseVec = generateNoiseVec(X_halfWidth * 2 + 1, 2 * Z_halfWidth + 1);

    for(int z = -(int)Z_halfWidth ; z <= (int)Z_halfWidth; z++)
    {
        for (int x = -(int)X_halfWidth; x <= (int)X_halfWidth; x++)
        {
            int gridIndex = (z + (int)Z_halfWidth) * (X_halfWidth * 2 + 1) + (x + (int)X_halfWidth);
            std::vector<int> heightLevels = GetHeightLevelsForChunk(x + (int)X_halfWidth, 
                        z + (int)Z_halfWidth, X_halfWidth * 2 + 1, 2 * Z_halfWidth + 1, gridOfNoiseVec);

            grid->gridOfChunks[gridIndex] = CreateChunk(device, x, z, heightLevels);
        }
    }

    for (int z = -(int)Z_halfWidth; z <= (int)Z_halfWidth; z++)
    {
        for (int x = -(int)X_halfWidth; x <= (int)X_halfWidth; x++)
        {
            int gridIndex = (z + (int)Z_halfWidth) * (X_halfWidth * 2 + 1) + (x + (int)X_halfWidth);
            Chunk* leftNeighbour = x > -(int)X_halfWidth ? grid->gridOfChunks[gridIndex - 1] : nullptr;
            Chunk* rightNeighbour = x < (int)X_halfWidth ? grid->gridOfChunks[gridIndex + 1] : nullptr;

            Chunk* frontNeighbour = z > -(int)Z_halfWidth ? grid->gridOfChunks[gridIndex - (X_halfWidth * 2 + 1)] : nullptr;
            Chunk* backNeighbour =  z < (int)Z_halfWidth ? grid->gridOfChunks[gridIndex + (X_halfWidth * 2 + 1)] : nullptr;

            UpdateGpuMemory(grid->gridOfChunks[gridIndex], leftNeighbour, rightNeighbour, backNeighbour, frontNeighbour);

        }
    }
    return grid;
}
