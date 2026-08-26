#include "FireCommon.hlsli"

FireDrawVertexOut main(Particle input)
{
    FireDrawVertexOut output;
    float time = input.Age;

    output.PosW =
        0.5f * time * time * gAccelW +
        time * input.InitialVelW +
        input.InitialPosW;
    output.Color = float4(
        1.0f,
        1.0f,
        1.0f,
        1.0f - smoothstep(0.0f, 1.0f, time));
    output.SizeW = input.SizeW;
    output.Type = input.Type;
    return output;
}
