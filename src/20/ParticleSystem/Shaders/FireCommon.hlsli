cbuffer cbPerFrame : register(b0)
{
    float3 gEyePosW;
    float3 gEmitPosW;
    float3 gEmitDirW;
    float gGameTime;
    float gTimeStep;
    row_major float4x4 gViewProj;
};

Texture2DArray gTexArray : register(t0);
Texture1D gRandomTex : register(t1);
SamplerState samLinear : register(s0);

static const uint PT_EMITTER = 0;
static const uint PT_FLARE = 1;
static const float3 gAccelW = float3(0.0f, 7.8f, 0.0f);
static const float2 gQuadTexC[4] =
{
    float2(0.0f, 1.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f)
};

struct Particle
{
    float3 InitialPosW : POSITION;
    float3 InitialVelW : VELOCITY;
    float2 SizeW : SIZE;
    float Age : AGE;
    uint Type : TYPE;
};

struct FireDrawVertexOut
{
    float3 PosW : POSITION;
    float2 SizeW : SIZE;
    float4 Color : COLOR;
    uint Type : TYPE;
};

struct FireGeoOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
    float2 Tex : TEXCOORD;
};

float3 RandUnitVec3(float offset)
{
    float3 randomVector =
        gRandomTex.SampleLevel(samLinear, gGameTime + offset, 0).xyz;
    return normalize(randomVector);
}
