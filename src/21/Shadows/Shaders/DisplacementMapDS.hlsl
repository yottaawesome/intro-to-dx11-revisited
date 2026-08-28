#include "DisplacementMapCommon.hlsli"

Texture2D gNormalMap : register(t1);

SamplerState samLinear : register(s0);

[domain("tri")]
DomainOut main(
    PatchTess patchTess,
    float3 barycentricCoordinates : SV_DomainLocation,
    const OutputPatch<HullOut, 3> patch)
{
    DomainOut dout;

    dout.PosW =
        barycentricCoordinates.x * patch[0].PosW +
        barycentricCoordinates.y * patch[1].PosW +
        barycentricCoordinates.z * patch[2].PosW;
    dout.NormalW =
        barycentricCoordinates.x * patch[0].NormalW +
        barycentricCoordinates.y * patch[1].NormalW +
        barycentricCoordinates.z * patch[2].NormalW;
    dout.TangentW =
        barycentricCoordinates.x * patch[0].TangentW +
        barycentricCoordinates.y * patch[1].TangentW +
        barycentricCoordinates.z * patch[2].TangentW;
    dout.Tex =
        barycentricCoordinates.x * patch[0].Tex +
        barycentricCoordinates.y * patch[1].Tex +
        barycentricCoordinates.z * patch[2].Tex;

    dout.NormalW = normalize(dout.NormalW);

    const float mipInterval = 20.0f;
    float mipLevel = clamp(
        (distance(dout.PosW, gEyePosW) - mipInterval) / mipInterval,
        0.0f,
        6.0f);

    float height = gNormalMap.SampleLevel(samLinear, dout.Tex, mipLevel).a;
    dout.PosW += (gHeightScale * (height - 1.0f)) * dout.NormalW;

    dout.ShadowPosH =
        mul(float4(dout.PosW, 1.0f), gShadowTransform);
    dout.PosH = mul(float4(dout.PosW, 1.0f), gViewProj);

    return dout;
}
