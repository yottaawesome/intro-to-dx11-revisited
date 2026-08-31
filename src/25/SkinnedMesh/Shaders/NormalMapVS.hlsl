#include "NormalMapCommon.hlsli"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
    float4 TangentL : TANGENT;
};

VertexOut main(VertexIn vin)
{
    VertexOut vout;

    vout.PosW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorldInvTranspose);
    vout.TangentW = mul(vin.TangentL, gWorld);
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
    vout.Tex = mul(float4(vin.Tex, 0.0f, 1.0f), gTexTransform).xy;
    vout.ShadowPosH = mul(float4(vin.PosL, 1.0f), gShadowTransform);
    vout.SsaoPosH = mul(float4(vin.PosL, 1.0f), gWorldViewProjTex);

    return vout;
}
