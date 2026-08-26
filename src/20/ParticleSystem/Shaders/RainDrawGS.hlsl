#include "RainCommon.hlsli"

[maxvertexcount(2)]
void main(
    point RainDrawVertexOut input[1],
    inout LineStream<RainGeoOut> lineStream)
{
    if (input[0].Type == PT_EMITTER)
    {
        return;
    }

    float3 startPosition = input[0].PosW;
    float3 endPosition = input[0].PosW + 0.07f * gAccelW;

    RainGeoOut startVertex;
    startVertex.PosH = mul(float4(startPosition, 1.0f), gViewProj);
    startVertex.Tex = float2(0.0f, 0.0f);
    lineStream.Append(startVertex);

    RainGeoOut endVertex;
    endVertex.PosH = mul(float4(endPosition, 1.0f), gViewProj);
    endVertex.Tex = float2(1.0f, 1.0f);
    lineStream.Append(endVertex);
}
