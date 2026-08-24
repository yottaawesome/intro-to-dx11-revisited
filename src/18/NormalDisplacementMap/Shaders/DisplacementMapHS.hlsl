#include "DisplacementMapCommon.hlsli"

PatchTess PatchHS(InputPatch<VertexOut, 3> patch, uint patchID : SV_PrimitiveID)
{
    PatchTess pt;

    pt.EdgeTess[0] = 0.5f * (patch[1].TessFactor + patch[2].TessFactor);
    pt.EdgeTess[1] = 0.5f * (patch[2].TessFactor + patch[0].TessFactor);
    pt.EdgeTess[2] = 0.5f * (patch[0].TessFactor + patch[1].TessFactor);
    pt.InsideTess = pt.EdgeTess[0];

    return pt;
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
