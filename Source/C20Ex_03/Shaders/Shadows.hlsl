

#include "common.hlsl"

#undef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 0

#undef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 1

struct VertexIn
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 TexC	   : TEXCOORD;
	float3 TangentU : TANGENT;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION1;
	float3 NormalW : NORMAL;
	float3 TangentW : TANGENT;
	float2 TexC	   : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	MaterialData matData = gMaterialData[gMaterialIndex];

	float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
	vout.PosW = posW.xyz;

	vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

	vout.TangentW = mul(vin.TangentU, (float3x3)gWorld);

	vout.PosH = mul(posW, gViewProj);

	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, matData.MatTransform).xy;

	return vout;
}

//float4 PS(VertexOut pin) : SV_Target
//{
//	MaterialData matData = gMaterialData[gMaterialIndex];
//	float4 diffuseAlbedo = matData.DiffuseAlbedo;
//	uint diffuseMapIndex = matData.DiffuseMapIndex;
//
//	diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);
//
//#ifdef ALPHA_TEST
//	clip(diffuseAlbedo.a - 0.1f);
//#endif
//	return diffuseAlbedo;
//}

float4 PS(VertexOut pin) : SV_Target
{
	MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = matData.DiffuseAlbedo;
	float3 fresnelR0 = matData.FresnelR0;
	float roughness = matData.Roughness;
	uint diffuseTexIndex = matData.DiffuseMapIndex;

	#ifdef ALPHA_TEST
	clip(diffuseAlbedo.a - 0.1f);
	#endif

	pin.NormalW = normalize(pin.NormalW);

	float3 toEyeW = normalize(gEyePosW - pin.PosW);

	float4 ambient = gAmbientLight * diffuseAlbedo;

	const float shininess = (1.0f - roughness);
	Material mat = { diffuseAlbedo, fresnelR0, shininess };

	float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);

	float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
		pin.NormalW, toEyeW, shadowFactor);

	float4 litColor = ambient + directLight;

//	litColor.a = diffuseAlbedo.a;

	return litColor;
}