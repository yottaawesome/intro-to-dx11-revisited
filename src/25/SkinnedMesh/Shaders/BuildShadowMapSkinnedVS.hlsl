#include "BuildShadowMapCommon.hlsli"

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

    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        posL += weights[i] * mul(
            float4(vin.PosL, 1.0f),
            gBoneTransforms[vin.BoneIndices[i]]).xyz;
    }

    vout.PosH = mul(float4(posL, 1.0f), gWorldViewProj);
    vout.Tex = mul(float4(vin.Tex, 0.0f, 1.0f), gTexTransform).xy;

    return vout;
}
