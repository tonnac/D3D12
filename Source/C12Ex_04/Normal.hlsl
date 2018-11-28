#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif


#include "LightingUtil.hlsl"

Texture2D    gDiffuseMap			: register(t0);

SamplerState gsamPointWrap			: register(s0);
SamplerState gsamPointClamp			: register(s1);
SamplerState gsamLinearWrap			: register(s2);
SamplerState gsamLinearClamp		: register(s3);
SamplerState gsamAnisotropicWrap    : register(s4);
SamplerState gsamAnisotropicClamp   : register(s5);

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gTexTransform;
};

cbuffer cbMaterial : register(b1)
{
	float4 gDiffuseAlbedo;
	float3 gFresnelR0;
	float  gRoughness;
	float4x4 gMatTransform;
}

cbuffer cbPass : register(b2)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float3 gEyePosW;
	float cbPerObjectPad1;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
	float4 gAmbientLight;

	float4 gFogColor;
	float gFogStart;
	float gFogRange;
	float2 cbPerObjectPad2;

	Light gLights[MaxLights];
};

struct VertexInOut
{
	float3 PosL		: POSITION;
	float3 NormalL	: NORMAL;
	float2 TexC		: TEXCOORD;
};

struct GeoOut
{
	float4 PosH		: SV_POSITION;
	float3 PosW		: POSITION;
	float3 NormalW	: NORMAL;
	float2 TexC		: TEXCOORD;
};

VertexInOut VS(VertexInOut vin)
{
	return vin;
}

[maxvertexcount(2)]
void GS(point VertexInOut gIn[1],
	inout LineStream<GeoOut> lineStream)
{
	GeoOut gOut[2];
	gOut[0].PosW = mul(gIn[0].PosL, (float3x3)gWorld);
	gOut[0].PosH = mul(float4(gOut[0].PosW, 1.0f), gViewProj);
	gOut[0].NormalW = gIn[0].NormalL;
	gOut[0].TexC = gIn[0].TexC;

	float3 Pos = gIn[0].PosL + (gIn[0].NormalL * 2.0f);

	gOut[1].PosW = mul(Pos, (float3x3)gWorld);
	gOut[1].PosH = mul(float4(gOut[1].PosW, 1.0f), gViewProj);
	gOut[1].NormalW = gOut[0].NormalW;
	gOut[1].TexC = gOut[0].TexC;


	lineStream.Append(gOut[0]);
	lineStream.Append(gOut[1]);
}

float4 PS(GeoOut pin) : SV_Target
{
	return float4(1.0f, 0.0f, 0.0f, 1.0f);
}


