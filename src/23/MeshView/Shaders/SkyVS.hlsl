#include "SkyCommon.hlsli"

cbuffer cbPerFrame : register(b0)
{
    row_major float4x4 gWorldViewProj;
};

struct VertexIn
{
    float3 PosL : POSITION;
};

VertexOut main(VertexIn vin)
{
    VertexOut vout;

    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj).xyww;
    vout.PosL = vin.PosL;

    return vout;
}
