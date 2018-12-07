//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************
 
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

struct InstanceData
{
	float4x4 World;
	float4x4 TexTransform;
	uint	 MaterialIndex;
	uint	 InstPad0;
	uint	 InstPad1;
	uint	 InstPad2;
	float4	 Color;
};

struct MaterialData
{
	float4	 DiffuseAlbedo;
	float3	 FresnelR0;
	float	 Roughness;
	float4x4 MatTransform;
	uint	 DiffuseMapIndex;
	uint	 MatPad0;
	uint	 MatPad1;
	uint	 MatPad2;
};

Texture2D    gDiffuseMap[7]			: register(t0);

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<MaterialData> gMaterialData : register(t1, space1);

SamplerState gsamPointWrap			: register(s0);
SamplerState gsamPointClamp			: register(s1);
SamplerState gsamLinearWrap			: register(s2);
SamplerState gsamLinearClamp		: register(s3);
SamplerState gsamAnisotropicWrap    : register(s4);
SamplerState gsamAnisotropicClamp   : register(s5);

cbuffer cbPass : register(b0)
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

	Light gLights[MaxLights];
};

struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC	   : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION;
	float3 NormalW : NORMAL;
	float2 TexC	   : TEXCOORD;
	float4 Color   : COLOR;

	nointerpolation uint MatIndex : MATINDEX;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
	VertexOut vout = (VertexOut)0.0f;

	InstanceData instData = gInstanceData[instanceID];
	float4x4 world = instData.World;
	float4x4 texTransform = instData.TexTransform;
	uint matIndex = instData.MaterialIndex;

	vout.MatIndex = matIndex;

	MaterialData matData = gMaterialData[matIndex];

	float4 posW = mul(float4(vin.PosL, 1.0f), world);
	vout.PosW = posW.xyz;

	vout.NormalW = mul(vin.NormalL, (float3x3)world);

	vout.PosH = mul(posW, gViewProj);

	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), texTransform);
	vout.TexC = mul(texC, matData.MatTransform).xy;

	vout.Color = instData.Color;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	MaterialData matData = gMaterialData[pin.MatIndex];
	float4 diffuseAlbedo = matData.DiffuseAlbedo;
	float3 fresnelR0 = matData.FresnelR0;
	float roughness = matData.Roughness;
	uint diffuseTexIndex = matData.DiffuseMapIndex;

	diffuseAlbedo *= gDiffuseMap[diffuseTexIndex].Sample(gsamLinearWrap, pin.TexC);

	pin.NormalW = normalize(pin.NormalW);

	float3 toEyeW = normalize(gEyePosW - pin.PosW);

	float4 ambient = gAmbientLight * diffuseAlbedo;

	const float shininess = 1.0f - roughness;
	Material mat = { diffuseAlbedo, fresnelR0, shininess };

	float3 shadowFactor = 1.0f;
	float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
		pin.NormalW, toEyeW, shadowFactor);

	float4 litColor = ambient + directLight;

	litColor.a = diffuseAlbedo.a;

	return litColor * pin.Color;
}


