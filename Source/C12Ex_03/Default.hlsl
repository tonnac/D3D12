//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Default shader, currently supports lighting.
//***************************************************************************************

// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

// Include structures and functions for lighting.
#include "LightingUtil.hlsl"

Texture2D    gDiffuseMap			: register(t0);

SamplerState gsamPointWrap			: register(s0);
SamplerState gsamPointClamp			: register(s1);
SamplerState gsamLinearWrap			: register(s2);
SamplerState gsamLinearClamp		: register(s3);
SamplerState gsamAnisotropicWrap    : register(s4);
SamplerState gsamAnisotropicClamp   : register(s5);


// Constant data that varies per frame.
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
};

// Constant data that varies per material.
cbuffer cbPass : register(b1)
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

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light gLights[MaxLights];
};

cbuffer cbMaterial : register(b2)
{
	float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float  gRoughness;
    float4x4 gMatTransform;
};

struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct GeoOut
{
	float4 PosH		: SV_POSITION;
	float3 PosW		: POSITION;
	float3 NormalW	: NORMAL;
	float2 TexC		: TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	vout.PosW = vin.PosL;
	vout.NormalW = vin.NormalL;
	
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, gMatTransform).xy;

    return vout;
}

[maxvertexcount(3)]
void GS(in triangle VertexOut gIn[3],
		uint primID : SV_PrimitiveID,
		inout TriangleStream<GeoOut> triStream)
{
	GeoOut gOut[3];
	for (int i = 0; i < 3; ++i)
	{
		gOut[i].NormalW = mul(gIn[i].NormalW, (float3x3)gWorld);

		float3 MovePos = gOut[i].NormalW * gTotalTime * primID / primID;

		gOut[i].PosW = mul(float4(gIn[i].PosW + MovePos, 1.0f), gWorld).xyz;
		gOut[i].TexC = gIn[i].TexC;
		gOut[i].PosH = mul(float4(gOut[i].PosW, 1.0f), gViewProj);
	}
	triStream.Append(gOut[0]);
	triStream.Append(gOut[1]);
	triStream.Append(gOut[2]);
	triStream.RestartStrip();
}

float4 PS(GeoOut pin) : SV_Target
{
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamPointWrap, pin.TexC) * gDiffuseAlbedo;

    pin.NormalW = normalize(pin.NormalW);

    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    float4 ambient = gAmbientLight*diffuseAlbedo;

    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    litColor.a = diffuseAlbedo.a;

    return litColor;
}


