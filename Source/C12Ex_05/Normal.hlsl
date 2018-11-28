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
void GS(triangle VertexInOut gIn[3],
	inout LineStream<GeoOut> lineStream)
{
	GeoOut gOut[2];

	float3 MidPos = (gIn[0].PosL + gIn[1].PosL + gIn[2].PosL) / 3;
	float3 MidNormal = (gIn[0].NormalL + gIn[1].NormalL + gIn[2].NormalL) / 3;

	gOut[0].PosW = mul(MidPos, (float3x3)gWorld);
	gOut[0].PosH = mul(float4(gOut[0].PosW, 1.0f), gViewProj);
	gOut[0].NormalW = MidNormal;
	gOut[0].TexC = float2(0.0f, 0.0f);

	gOut[1].PosW = mul(MidPos + MidNormal * 3.5f, (float3x3)gWorld);
	gOut[1].PosH = mul(float4(gOut[1].PosW, 1.0f), gViewProj);
	gOut[1].NormalW = MidNormal;
	gOut[1].TexC = float2(0.0f, 0.0f);

	lineStream.Append(gOut[0]);
	lineStream.Append(gOut[1]);
}

float4 PS(GeoOut pin) : SV_Target
{
	return float4(0.0f, 0.0f, 1.0f, 1.0f);
}


