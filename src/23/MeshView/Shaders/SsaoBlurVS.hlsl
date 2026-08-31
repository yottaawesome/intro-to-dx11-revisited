#include "SsaoBlurCommon.hlsli"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
};

VertexOut main(VertexIn vin)
{
    VertexOut vout;

    vout.PosH = float4(vin.PosL, 1.0f);
    vout.Tex = vin.Tex;

    return vout;
}
