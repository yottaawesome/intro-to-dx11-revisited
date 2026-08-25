#include "TerrainCommon.hlsli"

VertexOut main(VertexIn vin)
{
    VertexOut vout;
	
	// Terrain specified directly in world space.
    vout.PosW = vin.PosL;

	// Displace the patch corners to world space.  This is to make 
	// the eye to patch distance calculation more accurate.
    vout.PosW.y = gHeightMap.SampleLevel(samHeightmap, vin.Tex, 0).r;

	// Output vertex attributes to next stage.
    vout.Tex = vin.Tex;
    vout.BoundsY = vin.BoundsY;
	
    return vout;
}