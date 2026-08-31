#include "SsaoCommon.hlsli"

Texture2D gNormalDepthMap : register(t0);
Texture2D gRandomVecMap : register(t1);

SamplerState samNormalDepth : register(s0);
SamplerState samRandomVec : register(s1);

float OcclusionFunction(float distanceZ)
{
    float occlusion = 0.0f;
    if (distanceZ > gSurfaceEpsilon)
    {
        float fadeLength = gOcclusionFadeEnd - gOcclusionFadeStart;
        occlusion =
            saturate((gOcclusionFadeEnd - distanceZ) / fadeLength);
    }

    return occlusion;
}

float4 main(VertexOut pin) : SV_Target
{
    float4 normalDepth =
        gNormalDepthMap.SampleLevel(samNormalDepth, pin.Tex, 0.0f);

    float3 normal = normalDepth.xyz;
    float viewDepth = normalDepth.w;
    float3 position =
        (viewDepth / pin.ToFarPlane.z) * pin.ToFarPlane;

    float3 randomVector =
        2.0f * gRandomVecMap.SampleLevel(
            samRandomVec,
            4.0f * pin.Tex,
            0.0f).rgb - 1.0f;

    static const uint sampleCount = 14;
    float occlusionSum = 0.0f;

    [unroll]
    for (uint i = 0; i < sampleCount; ++i)
    {
        float3 offset = reflect(gOffsetVectors[i].xyz, randomVector);
        float flip = sign(dot(offset, normal));
        float3 samplePosition =
            position + flip * gOcclusionRadius * offset;

        float4 projectedSample =
            mul(float4(samplePosition, 1.0f), gViewToTexSpace);
        projectedSample /= projectedSample.w;

        float nearestDepth = gNormalDepthMap.SampleLevel(
            samNormalDepth,
            projectedSample.xy,
            0.0f).a;
        float3 nearestPosition =
            (nearestDepth / samplePosition.z) * samplePosition;

        float distanceZ = position.z - nearestPosition.z;
        float normalAlignment = max(
            dot(normal, normalize(nearestPosition - position)),
            0.0f);
        occlusionSum +=
            normalAlignment * OcclusionFunction(distanceZ);
    }

    occlusionSum /= sampleCount;

    float ambientAccess = 1.0f - occlusionSum;
    return saturate(pow(ambientAccess, 4.0f));
}
