
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
};

struct VertexIn
{
	float3 PosL : POSITION;
};

struct VertexOut
{
	float3 PosL : POSITION;
};

VertexOut VS(VertexIn vIn)
{
	VertexOut vOut = (VertexOut)0.0f;
	vOut.PosL = vIn.PosL;
	return vOut;
}

struct PatchTess
{
	float EdgeTess[4]	: SV_TessFactor;
	float InsideTess[2] : SV_InsideTessFactor;
};

PatchTess ConstantHS(InputPatch<VertexOut, 16> patch, uint patchID : SV_PrimitiveID)
{
	PatchTess pt;

	float3 centerL = 0.25f*(patch[0].PosL + patch[3].PosL + patch[12].PosL + patch[15].PosL);
	float3 centerW = mul(float4(centerL, 1.0f), gWorld).xyz;

	float d = distance(centerW, gEyePosW);

	const float d0 = 20.0f;
	const float d1 = 100.0f;

	float tess = 25.0f * saturate((d1 - d) / (d1 - d0));

	pt.EdgeTess[0] = tess;
	pt.EdgeTess[1] = tess;
	pt.EdgeTess[2] = tess;
	pt.EdgeTess[3] = tess;

	pt.InsideTess[0] = tess;
	pt.InsideTess[1] = tess;

	//pt.EdgeTess[0] = 25;
	//pt.EdgeTess[1] = 25;
	//pt.EdgeTess[2] = 25;
	//pt.EdgeTess[3] = 25;

	//pt.InsideTess[0] = 25;
	//pt.InsideTess[1] = 25;

	return pt;
}

struct HullOut
{
	float3 PosL : POSITION;
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(16)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 16> p,
			uint i : SV_OutputControlPointID,
			uint patchId : SV_PrimitiveID)
{
	HullOut hout;

	hout.PosL = p[i].PosL;

	return hout;
}

struct DomainOut
{
	float4 PosH : SV_POSITION;
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
	const OutputPatch<HullOut, 16> bezPatch)
{
	DomainOut dOut;

	float4 basisU = BernsteinBasis(uv.x);
	float4 basisV = BernsteinBasis(uv.y);

	float3 p = CubicBezierSum(bezPatch, basisU, basisV);

	float4 PosW = mul(float4(p, 1.0f), gWorld);
	dOut.PosH = mul(PosW, gViewProj);

	return dOut;
}

float4 PS(DomainOut pIn) : SV_Target
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}