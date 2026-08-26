#include "RainCommon.hlsli"

RainDrawVertexOut main(Particle input)
{
    RainDrawVertexOut output;
    float time = input.Age;
    output.PosW =
        0.5f * time * time * gAccelW +
        time * input.InitialVelW +
        input.InitialPosW;
    output.Type = input.Type;
    return output;
}
