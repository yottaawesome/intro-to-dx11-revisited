#include "TerrainCommon.hlsli"

float CalcTessFactor(float3 position)
{
    float distanceToEye = distance(position, gEyePosW);
    float interpolation = saturate(
        (distanceToEye - gMinDist) / (gMaxDist - gMinDist));
    return pow(2.0f, lerp(gMaxTess, gMinTess, interpolation));
}

bool AabbBehindPlane(float3 center, float3 extents, float4 plane)
{
    float radius = dot(extents, abs(plane.xyz));
    float signedDistance = dot(float4(center, 1.0f), plane);
    return signedDistance + radius < 0.0f;
}

bool AabbOutsideFrustum(
    float3 center,
    float3 extents,
    float4 frustumPlanes[6])
{
    for (int i = 0; i < 6; ++i)
    {
        if (AabbBehindPlane(center, extents, frustumPlanes[i]))
        {
            return true;
        }
    }
    return false;
}

TerrainPatchTess ConstantHS(
    InputPatch<TerrainVertexOut, 4> patch,
    uint patchId : SV_PrimitiveID)
{
    TerrainPatchTess tess;

    float minY = patch[0].BoundsY.x;
    float maxY = patch[0].BoundsY.y;
    float3 minBounds = float3(patch[2].PosW.x, minY, patch[2].PosW.z);
    float3 maxBounds = float3(patch[1].PosW.x, maxY, patch[1].PosW.z);
    float3 center = 0.5f * (minBounds + maxBounds);
    float3 extents = 0.5f * (maxBounds - minBounds);

    if (AabbOutsideFrustum(center, extents, gWorldFrustumPlanes))
    {
        [unroll]
        for (int i = 0; i < 4; ++i)
        {
            tess.EdgeTess[i] = 0.0f;
        }
        tess.InsideTess[0] = 0.0f;
        tess.InsideTess[1] = 0.0f;
        return tess;
    }

    float3 edge0 = 0.5f * (patch[0].PosW + patch[2].PosW);
    float3 edge1 = 0.5f * (patch[0].PosW + patch[1].PosW);
    float3 edge2 = 0.5f * (patch[1].PosW + patch[3].PosW);
    float3 edge3 = 0.5f * (patch[2].PosW + patch[3].PosW);
    float3 patchCenter =
        0.25f * (patch[0].PosW + patch[1].PosW + patch[2].PosW + patch[3].PosW);

    tess.EdgeTess[0] = CalcTessFactor(edge0);
    tess.EdgeTess[1] = CalcTessFactor(edge1);
    tess.EdgeTess[2] = CalcTessFactor(edge2);
    tess.EdgeTess[3] = CalcTessFactor(edge3);
    tess.InsideTess[0] = CalcTessFactor(patchCenter);
    tess.InsideTess[1] = tess.InsideTess[0];
    return tess;
}

[domain("quad")]
[partitioning("fractional_even")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
TerrainHullOut main(
    InputPatch<TerrainVertexOut, 4> patch,
    uint controlPointId : SV_OutputControlPointID,
    uint patchId : SV_PrimitiveID)
{
    TerrainHullOut output;
    output.PosW = patch[controlPointId].PosW;
    output.Tex = patch[controlPointId].Tex;
    return output;
}
