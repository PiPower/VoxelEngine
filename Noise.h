#pragma once
struct Vector2D
{
    float x, y;
};



float perlinStep(float grid_x, float grid_z);
float gnoise(float x, float y, float z);