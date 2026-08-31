cbuffer cbPerFrame : register(b0)
{
    float gTexelWidth;
    float gTexelHeight;
    float2 gPadding;
};

Texture2D gNormalDepthMap : register(t0);
Texture2D gInputImage : register(t1);

SamplerState samNormalDepth : register(s0);
SamplerState samInputImage : register(s1);

static const float gWeights[11] =
{
    0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.2f,
    0.1f, 0.1f, 0.1f, 0.05f, 0.05f
};
static const int gBlurRadius = 5;

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEXCOORD;
};

float4 Blur(VertexOut pin, float2 texelOffset)
{
    float4 color =
        gWeights[gBlurRadius] *
        gInputImage.SampleLevel(samInputImage, pin.Tex, 0.0f);
    float totalWeight = gWeights[gBlurRadius];

    float4 centerNormalDepth =
        gNormalDepthMap.SampleLevel(samNormalDepth, pin.Tex, 0.0f);

    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        if (i == 0)
        {
            continue;
        }

        float2 tex = pin.Tex + i * texelOffset;
        float4 neighborNormalDepth =
            gNormalDepthMap.SampleLevel(samNormalDepth, tex, 0.0f);

        if (dot(neighborNormalDepth.xyz, centerNormalDepth.xyz) >= 0.8f &&
            abs(neighborNormalDepth.a - centerNormalDepth.a) <= 0.2f)
        {
            float weight = gWeights[i + gBlurRadius];
            color += weight *
                gInputImage.SampleLevel(samInputImage, tex, 0.0f);
            totalWeight += weight;
        }
    }

    return color / totalWeight;
}
