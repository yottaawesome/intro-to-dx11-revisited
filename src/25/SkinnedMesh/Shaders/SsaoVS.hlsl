#include "SsaoCommon.hlsli"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 ToFarPlaneIndex : NORMAL;
    float2 Tex : TEXCOORD;
};

VertexOut main(VertexIn vin)
{
    VertexOut vout;

    vout.PosH = float4(vin.PosL, 1.0f);
    vout.ToFarPlane =
        gFrustumCorners[(uint)vin.ToFarPlaneIndex.x].xyz;
    vout.Tex = vin.Tex;

    return vout;
}
