#include "TerrainCommon.hlsli"

TerrainVertexOut main(TerrainVertexIn vin)
{
    TerrainVertexOut vout;
    vout.PosW = vin.PosL;
    vout.PosW.y = gHeightMap.SampleLevel(samHeightmap, vin.Tex, 0).r;
    vout.Tex = vin.Tex;
    vout.BoundsY = vin.BoundsY;
    return vout;
}
