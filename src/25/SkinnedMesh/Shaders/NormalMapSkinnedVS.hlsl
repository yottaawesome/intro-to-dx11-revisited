#include "NormalMapCommon.hlsli"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
    float4 TangentL : TANGENT;
    float3 Weights : WEIGHTS;
    uint4 BoneIndices : BONEINDICES;
};

VertexOut main(VertexIn vin)
{
    VertexOut vout;

    float4 weights = float4(
        vin.Weights,
        1.0f - vin.Weights.x - vin.Weights.y - vin.Weights.z);

    float3 posL = float3(0.0f, 0.0f, 0.0f);
    float3 normalL = float3(0.0f, 0.0f, 0.0f);
    float3 tangentL = float3(0.0f, 0.0f, 0.0f);

    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        row_major float4x4 boneTransform =
            gBoneTransforms[vin.BoneIndices[i]];
        posL += weights[i] * mul(float4(vin.PosL, 1.0f), boneTransform).xyz;
        normalL +=
            weights[i] * mul(vin.NormalL, (float3x3)boneTransform);
        tangentL +=
            weights[i] * mul(vin.TangentL.xyz, (float3x3)boneTransform);
    }

    vout.PosW = mul(float4(posL, 1.0f), gWorld).xyz;
    vout.NormalW = mul(normalL, (float3x3)gWorldInvTranspose);
    vout.TangentW =
        float4(mul(tangentL, (float3x3)gWorld), vin.TangentL.w);
    vout.PosH = mul(float4(posL, 1.0f), gWorldViewProj);
    vout.Tex = mul(float4(vin.Tex, 0.0f, 1.0f), gTexTransform).xy;
    vout.ShadowPosH = mul(float4(posL, 1.0f), gShadowTransform);
    vout.SsaoPosH = mul(float4(posL, 1.0f), gWorldViewProjTex);

    return vout;
}
