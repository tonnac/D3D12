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
	float4 PosW : POSITION1;
	float3 NormalL : NORMAL;
	float2 TexC : TEXCOORD;
	float3 TangentU : TANGENT;
};

VertexOut VS(in VertexIn vIn)
{
	VertexOut vOut = (VertexOut)0.0f;
	vOut.PosL = vIn.PosL;
	vOut.PosW = mul(float4(vIn.PosL, 1.0f), gWorld);
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
	
	float3 Center = (patch[0].PosW + patch[1].PosW + patch[2].PosW + patch[3].PosW).xyz / 4.0f;

	float d = distance(Center, gEyePosW);

	const float d0 = 20.0f;
	const float d1 = 100.0f;

	float tess = 64.0f * saturate((d1 - d) / (d1 - d0));

	pt.EdgeTess[0] = tess;
	pt.EdgeTess[1] = tess;
	pt.EdgeTess[2] = tess;
	pt.EdgeTess[3] = tess;

	pt.InsideTess[0] = tess;
	pt.InsideTess[1] = tess;

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
[partitioning("fractional_odd")]
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
	if (col == 1)
	{
		hOut.PosL.y = 3.0f * cos(gTotalTime);
	}
	else if (col == 2)
	{
		hOut.PosL.y = -3.0f * sin(gTotalTime);
	}
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

float4 BernsteinBasis(float t)
{
	float invT = 1.0f - t;

	return float4(
		invT * invT * invT,
		3.0f * t * invT * invT,
		3.0f * t * t * invT,
		t * t * t);
}

float3 CubicBezierSum(const OutputPatch<HullOut, 16> bezPatch, float4 basisU, float4 basisV)
{
	float3 sum = float3(0.0f, 0.0f, 0.0f);
	sum = basisV.x * ((basisU.x * bezPatch[0].PosL) + (basisU.y * bezPatch[1].PosL) + (basisU.z * bezPatch[2].PosL) + (basisU.w * bezPatch[3].PosL));
	sum += basisV.y * ((basisU.x * bezPatch[4].PosL) + (basisU.y * bezPatch[5].PosL) + (basisU.z * bezPatch[6].PosL) + (basisU.w * bezPatch[7].PosL));
	sum += basisV.z * ((basisU.x * bezPatch[8].PosL) + (basisU.y * bezPatch[9].PosL) + (basisU.z * bezPatch[10].PosL) + (basisU.w * bezPatch[11].PosL));
	sum += basisV.w * ((basisU.x * bezPatch[12].PosL) + (basisU.y * bezPatch[13].PosL) + (basisU.z * bezPatch[14].PosL) + (basisU.w * bezPatch[15].PosL));

	return sum;
}

float4 dBernsteinBasis(float t)
{
	float invT = 1.0f - t;

	return float4(
		-3 * invT * invT,
		3 * invT * (invT - 6) * t * invT,
		6 * t * (invT - 3) * t * t,
		3 * t * t);
}

[domain("quad")]
DomainOut DS(PatchTess patchTess,
	float2 uv : SV_DomainLocation,
	const OutputPatch<HullOut, 16> quad)
{
	DomainOut dOut;

	MaterialData matData = gMaterialData[gMaterialIndex];

	float4 basisU = BernsteinBasis(uv.x);
	float4 basisV = BernsteinBasis(uv.y);

	float4 dbasisU = dBernsteinBasis(uv.x);
	float4 dbasisV = dBernsteinBasis(uv.y);

	float3 p = CubicBezierSum(quad, basisU, basisV);

	float3 v1 = lerp(quad[0].PosL, quad[3].PosL, uv.x);
	float3 v2 = lerp(quad[12].PosL, quad[15].PosL, uv.x);

	float2 t1 = lerp(quad[0].TexC, quad[3].TexC, uv.x);
	float2 t2 = lerp(quad[12].TexC, quad[15].TexC, uv.x);

//	float3 p = lerp(v1, v2, uv.y);
	float2 t = lerp(t1, t2, uv.y);

	float2 tt = t - (gTotalTime * 0.005f);
	float2 tp = t + (gTotalTime * 0.005f);

	float4 col = gHeightMaps[2].SampleLevel(gsamAnisotropicWrap, tt, 0);
	col = 2.0f * col - 1.0f;
	float4 col1 = gHeightMaps[3].SampleLevel(gsamAnisotropicWrap, tp, 0);
	col1 = 2.0f * col1 - 1.0f;
	p.y += col.a + col1.a;

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


float3 BoxCubeMapLookup(float3 rayOrigin, float3 unitRayDir, float3 boxCenter, float3 boxExtents)
{
	float3 p = rayOrigin - boxCenter;

	float3 t1 = (-p + boxExtents) / unitRayDir;
	float3 t2 = (-p - boxExtents) / unitRayDir;

	float3 tmax = max(t1, t2);

	float t = min(min(tmax.x, tmax.y), tmax.z);

	return p + t * unitRayDir;
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

	float4 normalMapSample = gHeightMaps[1].Sample(gsamAnisotropicWrap, t1.xy);
	float4 normalMapSample1 = gHeightMaps[0].Sample(gsamAnisotropicWrap, t2.xy);
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
	float3 reflect = BoxCubeMapLookup(pin.PosW, r, float3(0, 0, 0), float3(5000, 5000, 5000));
	float3 re = refract(-toEyeW, bumpedNormalW, 1.33f);
	float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, reflect);
	float4 recolor = gCubeMap.Sample(gsamLinearWrap, re);
	float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, reflect);
	litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;// *recolor.rgb;

	litColor.a = diffuseAlbedo.a;

	return litColor;
}