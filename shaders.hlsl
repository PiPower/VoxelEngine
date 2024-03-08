struct Vin
{
    float3 pos : POSITION;
};

struct Vout
{
    float4 Pos : SV_Position;
};

Vout VS(Vin vin)
{
    Vout vout;
    vout.Pos = float4(vin.pos.x, vin.pos.y, 0, 1);
    return vout;
}

float4 PS(Vout vin) : SV_Target
{

    return float4(1.0f, 0.4f, 0.7f, 1.0f);

}
