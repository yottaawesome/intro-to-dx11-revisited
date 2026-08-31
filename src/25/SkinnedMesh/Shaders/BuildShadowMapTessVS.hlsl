#include "BuildShadowMapCommon.hlsli"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
};

TessVertexOut main(VertexIn vin)
{
    TessVertexOut vout;

    vout.PosW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorldInvTranspose);
    vout.Tex = mul(float4(vin.Tex, 0.0f, 1.0f), gTexTransform).xy;

    float distanceToEye = distance(vout.PosW, gEyePosW);
    float tessellationAmount = saturate(
        (gMinTessDistance - distanceToEye) /
        (gMinTessDistance - gMaxTessDistance));

    vout.TessFactor = gMinTessFactor +
        tessellationAmount * (gMaxTessFactor - gMinTessFactor);

    return vout;
}
