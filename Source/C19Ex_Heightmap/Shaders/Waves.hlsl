#include "common.hlsl"

struct VertexIn
{
	float3 PosL : POSITION;
	float3 NormalL : NORMAL;
	float2 TexC : TEXCOORD;
	float3 TangentU : TANGENT;
};

struct VertexOut
{
	float3 PosL : POSITION;
	float3 NormalL : NORMAL;
	float2 TexC : TEXCOORD;
	float3 TangentU : TANGENT;
};

VertexOut VS(in VertexIn vIn)
{
	VertexOut vOut = (VertexOut)0.0f;
	vOut.PosL = vIn.PosL;
	vOut.NormalL = vIn.NormalL;
	vOut.TexC = vIn.TexC;
	vOut.TangentU = vIn.TangentU;
	return vOut;
}

struct PatchTess
{
	float EdgeTess[4] : SV_TessFactor;
	float InsideTess[2] : SV_InsideTessFactor;
};

PatchTess ConstantHS(InputPatch<VertexOut, 4> patch, uint patchID : SV_PrimitiveID)
{
	PatchTess pt;

	pt.EdgeTess[0] = 64.0f;
	pt.EdgeTess[1] = 64.0f;
	pt.EdgeTess[2] = 64.0f;
	pt.EdgeTess[3] = 64.0f;

	pt.InsideTess[0] = 64.0f;
	pt.InsideTess[1] = 64.0f;

	return pt;
}

struct HullOut
{
	float3 PosL : POSITION;
	float3 NormalL : NORMAL;
	float2 TexC : TEXCOORD;
	float3 TangentU : TANGENT;
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(16)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 4> p,
	uint i : SV_outputControlPointID,
	uint patchID : SV_PrimitiveID)
{
	HullOut hOut;

	uint row = i / 4;
	uint col = i % 4;

	float ratioCol = (float)col / 3.0f;
	float ratioRow = (float)row / 3.0f;

	hOut.NormalL = p[0].NormalL;
	hOut.TangentU = p[0].TangentU;

	float3 v1 = p[1].PosL - p[0].PosL;
	float3 v2 = p[2].PosL - p[0].PosL;

	float2 t1 = lerp(p[0].TexC, p[1].TexC, ratioCol);
	float2 t2 = lerp(p[2].TexC, p[3].TexC, ratioCol);

	float2 Tex = lerp(t1, t2, ratioRow);

	float3 v4 = v1 * ratioCol;
	float3 v5 = v2 * ratioRow;

	hOut.PosL = p[0].PosL + v4 + v5;
	hOut.TexC = Tex;

	return hOut;
}

struct DomainOut
{
	float4 PosH : SV_POSITION;
	float3 PosW : POSITION;
	float3 NormalW : NORMAL;
	float2 TexC : TEXCOORD;
	float3 TangentW : TANGENT;
	float2 t0 : TEXCOORD1;
	float2 t1 : TEXCOORD2;
};

[domain("quad")]
DomainOut DS(PatchTess patchTess,
	float2 uv : SV_DomainLocation,
	const OutputPatch<HullOut, 16> quad)
{
	DomainOut dOut;

	MaterialData matData = gMaterialData[gMaterialIndex];

	float3 v1 = lerp(quad[0].PosL, quad[3].PosL, uv.x);
	float3 v2 = lerp(quad[12].PosL, quad[15].PosL, uv.x);

	float2 t1 = lerp(quad[0].TexC, quad[3].TexC, uv.x);
	float2 t2 = lerp(quad[12].TexC, quad[15].TexC, uv.x);

	float3 p = lerp(v1, v2, uv.y);
	float2 t = lerp(t1, t2, uv.y);

	float2 tt = t - (gTotalTime * 0.005f);
	float2 tp = t + (gTotalTime * 0.005f);

	float4 col = gHeightMaps[0].SampleLevel(gsamAnisotropicWrap, tt, 0);
	col = 2.0f * col - 1.0f;
	float4 col1 = gHeightMaps[1].SampleLevel(gsamAnisotropicWrap, tp, 0);
	col1 = 2.0f * col1 - 1.0f;
	p.y = col.a + col1.a;

	float4 PosW = mul(float4(p, 1.0f), gWorld);
	dOut.PosW = PosW.xyz;
	dOut.PosH = mul(PosW, gViewProj);
	float3 Normal = float3(0.0f, 1.0f, 0.0f);
	dOut.NormalW = mul(Normal, (float3x3)gWorld);
	dOut.TangentW = float3(1.0f, 0.0f, 0.0f);
	float4 texC = mul(float4(t, 0.0f, 1.0f), gTexTransform);
	dOut.TexC = texC.xy;
	dOut.t0 = tt;
	dOut.t1 = tp;
	return dOut;
}

float4 PS(DomainOut pin) : SV_Target
{
	MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = matData.DiffuseAlbedo;
	float3 fresnelR0 = matData.FresnelR0;
	float roughness = matData.Roughness;
	uint diffuseTexIndex = matData.DiffuseMapIndex;
	uint normalMapIndex = matData.NormalMapIndex;

	pin.NormalW = normalize(pin.NormalW);

	float4 t1 = mul(float4(pin.t0, 0.0f, 1.0f), gTexTransform);
	float4 t2 = mul(float4(pin.t1, 0.0f, 1.0f), gTexTransform);

	float4 normalMapSample = gTextureMaps[1].Sample(gsamAnisotropicWrap, t1.xy);
	float4 normalMapSample1 = gTextureMaps[0].Sample(gsamAnisotropicWrap, t2.xy);
	float3 bumpedNormalW0 = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);
	float3 bumpedNormalW1 = NormalSampleToWorldSpace(normalMapSample1.rgb, pin.NormalW, pin.TangentW);

	float3 bumpedNormalW = normalize(bumpedNormalW0 + bumpedNormalW1);

	diffuseAlbedo *= gTextureMaps[diffuseTexIndex].Sample(gsamLinearWrap, pin.TexC);

	float3 toEyeW = normalize(gEyePosW - pin.PosW);

	float4 ambient = gAmbientLight * diffuseAlbedo;

	const float shininess = (1.0f - roughness) * normalMapSample.a;
	Material mat = { diffuseAlbedo, fresnelR0, shininess };

	float3 shadowFactor = 1.0f;
	float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
		bumpedNormalW, toEyeW, shadowFactor);

	float4 litColor = ambient + directLight;

	float3 r = reflect(-toEyeW, bumpedNormalW);
	float3 re = refract(-toEyeW, bumpedNormalW, 1.33f);
	float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r);
	float4 recolor = gCubeMap.Sample(gsamLinearWrap, re);
	float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
	litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb * recolor.rgb;

	litColor.a = diffuseAlbedo.a;

	return litColor;
}