#include "RainCommon.hlsli"

[maxvertexcount(6)]
void main(
    point Particle input[1],
    inout PointStream<Particle> particleStream)
{
    input[0].Age += gTimeStep;

    if (input[0].Type == PT_EMITTER)
    {
        if (input[0].Age > 0.002f)
        {
            [unroll]
            for (int i = 0; i < 5; ++i)
            {
                float3 randomOffset = 35.0f * RandVec3((float)i / 5.0f);
                randomOffset.y = 20.0f;

                Particle particle;
                particle.InitialPosW = gEmitPosW + randomOffset;
                particle.InitialVelW = float3(0.0f, 0.0f, 0.0f);
                particle.SizeW = float2(1.0f, 1.0f);
                particle.Age = 0.0f;
                particle.Type = PT_FLARE;
                particleStream.Append(particle);
            }

            input[0].Age = 0.0f;
        }

        particleStream.Append(input[0]);
    }
    else if (input[0].Age <= 3.0f)
    {
        particleStream.Append(input[0]);
    }
}
