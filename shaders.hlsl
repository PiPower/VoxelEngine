#define FRONT_S (1.0f/3.0f)
#define FRONT_E (2.0f/3.0f)
#define RIGHT_S (1.0f/3.0f)
#define RIGHT_E (2.0f/3.0f)
#define LEFT_S (1.0f/3.0f)
#define LEFT_E (2.0f/3.0f)
#define BACK_S (1.0f/3.0f)
#define BACK_E (2.0f/3.0f)

#define TOP_S 0.6726f
#define TOP_E (1.0f)

#define BOTTOM_S 0 
#define BOTTOM_E (1.0f/3.0f)
struct Vin
{
    float3 pos : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct Vout
{
    float4 Pos : SV_Position;
    float2 texCoord : TEXCOORD0;
};


cbuffer WorldTransform : register(b0)
{
    float4x4 CameraTransform;
    float4x4 ProjectionTransform;
};

cbuffer ChunkCbuffer : register(b1)
{
    float2 chunkOffset;
};

Texture2D tex : register(t0);
SamplerState samplerWrap : register(s0);

static float2 firstVertexText[] =
{
    float2(RIGHT_S, 0),//right face
    float2(RIGHT_E, 0),//right face
    float2(LEFT_S, 0), //left face
    float2(LEFT_E, 0), //left face
    float2(TOP_S, 0), //top face
    float2(TOP_E, 0), //top face
    float2(BOTTOM_S, 0), //bottom face
    float2(BOTTOM_E, 0), //bottom face
    float2(BACK_S, 0), //back face
    float2(BACK_E, 0), //back face
};

static float2 secondVertexText[] =
{
    float2(RIGHT_E, 0), //right face
    float2(RIGHT_E, 1), //right face
    float2(LEFT_E, 0), //left face
    float2(LEFT_E, 1), //left face
    float2(TOP_E, 0), //top face
    float2(TOP_E, 1), //top face
    float2(BOTTOM_E, 0), //bottom face
    float2(BOTTOM_E, 1), //bottom face
    float2(BACK_E, 0), //back face
    float2(BACK_E, 1), //back face
};

static float2 thirdVertexText[] =
{
    float2(RIGHT_S, 1), //right face
    float2(RIGHT_S, 1), //right face
    float2(LEFT_S, 1), //left face
    float2(LEFT_S, 1), //left face
    float2(TOP_S, 1), //top face
    float2(TOP_S, 1), //top face
    float2(BOTTOM_S, 1), //bottom face
    float2(BOTTOM_S, 1), //bottom face
    float2(BACK_S, 1), //back face
    float2(BACK_S, 1), //back face
};

Vout VS(Vin vin)
{    
    Vout vout;
    vin.pos = vin.pos + float3(chunkOffset.x, 0, chunkOffset.y);
    vout.Pos = mul(float4(vin.pos, 1), CameraTransform);
    vout.Pos = mul(vout.Pos, ProjectionTransform);
    vout.texCoord = vin.texCoord;
    return vout;
}


float4 PS(Vout vin) : SV_Target
{
    return tex.Sample(samplerWrap, vin.texCoord);
}
