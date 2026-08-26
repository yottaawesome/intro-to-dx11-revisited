#include "FireCommon.hlsli"

[maxvertexcount(4)]
void main(
    point FireDrawVertexOut input[1],
    inout TriangleStream<FireGeoOut> triangleStream)
{
    if (input[0].Type == PT_EMITTER)
    {
        return;
    }

    float3 look = normalize(gEyePosW - input[0].PosW);
    float3 right = normalize(cross(float3(0.0f, 1.0f, 0.0f), look));
    float3 up = cross(look, right);

    float halfWidth = 0.5f * input[0].SizeW.x;
    float halfHeight = 0.5f * input[0].SizeW.y;

    float4 positions[4];
    positions[0] = float4(
        input[0].PosW + halfWidth * right - halfHeight * up, 1.0f);
    positions[1] = float4(
        input[0].PosW + halfWidth * right + halfHeight * up, 1.0f);
    positions[2] = float4(
        input[0].PosW - halfWidth * right - halfHeight * up, 1.0f);
    positions[3] = float4(
        input[0].PosW - halfWidth * right + halfHeight * up, 1.0f);

    FireGeoOut output;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        output.PosH = mul(positions[i], gViewProj);
        output.Color = input[0].Color;
        output.Tex = gQuadTexC[i];
        triangleStream.Append(output);
    }
}
