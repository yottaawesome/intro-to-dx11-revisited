#include "SsaoNormalDepthCommon.hlsli"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
};

VertexOut main(VertexIn vin)
{
    VertexOut vout;

    vout.PosV = mul(float4(vin.PosL, 1.0f), gWorldView).xyz;
    vout.NormalV =
        mul(vin.NormalL, (float3x3)gWorldInvTransposeView);
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
    vout.Tex = mul(float4(vin.Tex, 0.0f, 1.0f), gTexTransform).xy;

    return vout;
}
