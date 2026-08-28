#include "DisplacementMapCommon.hlsli"

PatchTess PatchHS(InputPatch<VertexOut, 3> patch, uint patchID : SV_PrimitiveID)
{
    PatchTess tessellation;

    tessellation.EdgeTess[0] =
        0.5f * (patch[1].TessFactor + patch[2].TessFactor);
    tessellation.EdgeTess[1] =
        0.5f * (patch[2].TessFactor + patch[0].TessFactor);
    tessellation.EdgeTess[2] =
        0.5f * (patch[0].TessFactor + patch[1].TessFactor);
    tessellation.InsideTess = tessellation.EdgeTess[0];

    return tessellation;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchHS")]
HullOut main(
    InputPatch<VertexOut, 3> patch,
    uint controlPointID : SV_OutputControlPointID,
    uint patchID : SV_PrimitiveID)
{
    HullOut hout;

    hout.PosW = patch[controlPointID].PosW;
    hout.NormalW = patch[controlPointID].NormalW;
    hout.TangentW = patch[controlPointID].TangentW;
    hout.Tex = patch[controlPointID].Tex;

    return hout;
}
