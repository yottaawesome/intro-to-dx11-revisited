#include "TerrainCommon.hlsli"

[domain("quad")]
TerrainDomainOut main(
    TerrainPatchTess patchTess,
    float2 uv : SV_DomainLocation,
    const OutputPatch<TerrainHullOut, 4> quad)
{
    TerrainDomainOut output;

    output.PosW = lerp(
        lerp(quad[0].PosW, quad[1].PosW, uv.x),
        lerp(quad[2].PosW, quad[3].PosW, uv.x),
        uv.y);
    output.Tex = lerp(
        lerp(quad[0].Tex, quad[1].Tex, uv.x),
        lerp(quad[2].Tex, quad[3].Tex, uv.x),
        uv.y);
    output.TiledTex = output.Tex * gTexScale;
    output.PosW.y = gHeightMap.SampleLevel(samHeightmap, output.Tex, 0).r;
    output.PosH = mul(float4(output.PosW, 1.0f), gViewProj);

    return output;
}
