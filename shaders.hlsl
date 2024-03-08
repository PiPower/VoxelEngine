struct Vin
{
    float3 pos : POSITION;
};

struct Vout
{
    float4 Pos : SV_Position;
};

cbuffer WorldTransform : register(b0)
{
    float4x4 CameraTransform;
    float4x4 ProjectionTransform;
};

static float4 triangleColor[] =
{
    float4(1, 1, 1, 1),
    float4(0.7, 0.2, 0.8, 1),
};

Vout VS(Vin vin)
{    
    Vout vout;
    vout.Pos = mul(float4(vin.pos, 1), CameraTransform);
    vout.Pos = mul(vout.Pos, ProjectionTransform);
    return vout;
}

float4 PS(Vout vin, uint primID : SV_PrimitiveID) : SV_Target
{

    return triangleColor[primID%2];

}
