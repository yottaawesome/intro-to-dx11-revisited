cbuffer cbPerFrame : register(b0)
{
	float3 gEyePosW;
    float gPadding;
};

cbuffer cbPerObject : register(b1)
{
	row_major float4x4 gWorld;
	row_major float4x4 gWorldViewProj;
};

struct VertexIn
{
	float3 PosL : POSITION;
};

struct VertexOut
{
	float3 PosL : POSITION;
};

struct PatchTess
{
	float EdgeTess[4] : SV_TessFactor;
	float InsideTess[2] : SV_InsideTessFactor;
};

struct HullOut
{
	float3 PosL : POSITION;
};

struct DomainOut
{
	float4 PosH : SV_POSITION;
};
