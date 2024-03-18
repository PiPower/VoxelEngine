#include "Noise.h"
#include <math.h>
#include <random>
#define TABSIZE 256

float* randomTable = nullptr;
int permTable[TABSIZE] = { 217, 0, 21, 102, 9, 53, 180, 106, 156, 5, 111, 69, 134, 74, 32, 212, 42, 40, 153, 195,
129, 145, 101, 136, 16, 143, 233, 203, 105, 75, 208, 137, 215, 199, 155, 159, 135, 2, 169, 148, 140, 168, 187, 226,
131, 147, 251, 158, 127, 92, 88, 175, 176, 165, 128, 174, 194, 198, 124, 11, 207, 119, 59, 225, 116, 201, 62, 151,
253, 172, 162, 237, 206, 229, 120, 45, 241, 28, 161, 41, 245, 183, 146, 71, 133, 238, 141, 211, 61, 177, 31, 1, 65,
86, 218, 83, 244, 93, 66, 152, 78, 36, 70, 182, 24, 185, 77, 160, 210, 138, 171, 17, 10, 14, 27, 214, 89, 250, 110,
193, 164, 39, 81, 230, 126, 98, 87, 54, 130, 190, 20, 234, 154, 196, 166, 255, 191, 72, 64, 231, 57, 170, 96, 63, 52,
37, 149, 205, 239, 247, 99, 117, 242, 235, 8, 150, 3, 118, 44, 73, 123, 192, 252, 189, 200, 202, 114, 108, 221, 254, 113,
84, 68, 19, 94, 188, 243, 157, 112, 232, 47, 4, 46, 223, 107, 23, 227, 248, 220, 142, 50, 178, 22, 132, 125, 95, 163, 43,
109, 222, 60, 209, 38, 90, 7, 104, 173, 51, 55, 216, 204, 12, 15, 249, 197, 224, 80, 144, 82, 184, 85, 25, 100, 121, 139,
186, 246, 67, 219, 6, 167, 58, 18, 79, 213, 30, 103, 181, 91, 29, 13, 33, 48, 179, 34, 228, 35, 76, 97, 122, 26, 56, 49, 236, 240, 115 };

static Vector2D randomGradient(int ix, int iy)
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


void generateEntriesInRandomTable()
{
    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_real_distribution<float> uniform_dist(-1, 1);
    randomTable = new float[TABSIZE];

    for (int i = 0; i < TABSIZE; i++)
    {
        randomTable[i] = uniform_dist(e1);
    }
}

float PERM(int x)
{
    int index = x &(TABSIZE - 1);
    return permTable[index];
}

float INDEX(int x,int y,int z) 
{
    return  PERM(PERM(x) + PERM(y) + PERM(z));
}

float LERP(float t, float x0, float xl)
{
    return (x0+t * (xl-x0));
}

float SMOOTHSTEP(float x)
{
    return x * x * (3 - 2 * x);
}

float glattice(int ix, int iy, int iz,  float fx, float fy, float fz)
{
    Vector2D grad = randomGradient(ix, iy);
    return grad.x * fx + grad.y * fy;
}

float gnoise(float x, float y, float z)
{
    if (randomTable == nullptr)
    {
        generateEntriesInRandomTable();
    }

    float ix = floor(x);
    float fx0 = x - ix;
    float fxl = fx0 - 1;
    float wx = SMOOTHSTEP(fx0);
     
    float iy = floor(y);
    float fy0 = y - iy;
    float fyl = fy0 - 1;
    float wy = SMOOTHSTEP(fy0);
     
    float iz = floor(z);
    float fz0 = z - iz;
    float fzl = fz0 - 1;
    float wz = SMOOTHSTEP(fz0);

    float vx0 = glattice(ix, iy, iz, fx0, fy0, fz0);
    float vxl = glattice(ix + 1, iy, iz, fxl, fy0, fz0);
    float vy0 = LERP(wx, vx0, vxl);
    vx0 = glattice(ix, iy + 1, iz, fx0, fyl, fz0);
    vxl = glattice(ix + 1, iy + 1, iz, fxl, fyl, fz0);
    float vyl = LERP(wx, vx0, vxl);
    float vz0 = LERP(wy, vy0, vyl);
    vx0 = glattice(ix, iy, iz + 1, fx0, fy0, fzl);
    vxl = glattice(ix + 1, iy, iz + 1, fxl, fy0, fzl);
    vy0 = LERP(wx, vx0, vxl);
    vx0 = glattice(ix, iy + 1, iz + 1, fx0, fyl, fzl);
    vxl = glattice(ix + 1, iy + 1, iz + 1, fxl, fyl, fzl);
    vyl = LERP(wx, vx0, vxl);
    float vzl = LERP(wy, vy0, vyl);
    return LERP(wz, vz0, vzl);
}


static float interpolate(float a0, float a1, float w) {
    return (a1 - a0) * ((w * (w * 6.0 - 15.0) + 10.0) * w * w * w) + a0;
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