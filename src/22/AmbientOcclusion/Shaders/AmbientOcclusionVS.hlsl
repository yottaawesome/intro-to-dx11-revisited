#include "AmbientOcclusionCommon.hlsli"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD0;
    float AmbientOcc : AMBIENT;
};

VertexOut main(VertexIn vin)
{
    VertexOut vout;

    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
    vout.AmbientOcc = vin.AmbientOcc;

    return vout;
}
