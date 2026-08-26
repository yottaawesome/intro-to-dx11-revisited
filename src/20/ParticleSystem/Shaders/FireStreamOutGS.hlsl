#include "FireCommon.hlsli"

[maxvertexcount(2)]
void main(
    point Particle input[1],
    inout PointStream<Particle> particleStream)
{
    input[0].Age += gTimeStep;

    if (input[0].Type == PT_EMITTER)
    {
        if (input[0].Age > 0.005f)
        {
            float3 randomVelocity = RandUnitVec3(0.0f);
            randomVelocity.x *= 0.5f;
            randomVelocity.z *= 0.5f;

            Particle particle;
            particle.InitialPosW = gEmitPosW;
            particle.InitialVelW = 4.0f * randomVelocity;
            particle.SizeW = float2(3.0f, 3.0f);
            particle.Age = 0.0f;
            particle.Type = PT_FLARE;
            particleStream.Append(particle);

            input[0].Age = 0.0f;
        }

        particleStream.Append(input[0]);
    }
    else if (input[0].Age <= 1.0f)
    {
        particleStream.Append(input[0]);
    }
}
