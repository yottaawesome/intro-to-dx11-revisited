cbuffer cbPerFrame : register(b0)
{
    float3 gEyePosW;
    float padA;
    float3 gEmitPosW;
    float padB;
    float3 gEmitDirW;
    float padC;
    float gGameTime;
    float gTimeStep;
    float2 padD;
    row_major float4x4 gViewProj;
};

Texture2DArray gTexArray : register(t0);
Texture1D gRandomTex : register(t1);
SamplerState samLinear : register(s0);

static const uint PT_EMITTER = 0;
static const uint PT_FLARE = 1;
static const float3 gAccelW = float3(-1.0f, -9.8f, 0.0f);

struct Particle
{
    float3 InitialPosW : POSITION;
    float3 InitialVelW : VELOCITY;
    float2 SizeW : SIZE;
    float Age : AGE;
    uint Type : TYPE;
};

struct RainDrawVertexOut
{
    float3 PosW : POSITION;
    uint Type : TYPE;
};

struct RainGeoOut
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEXCOORD;
};

float3 RandVec3(float offset)
{
    return gRandomTex.SampleLevel(samLinear, gGameTime + offset, 0).xyz;
}
