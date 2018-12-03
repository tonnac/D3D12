
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
	float EdgeTess[3]	: SV_TessFactor;
	float InsideTess[1] : SV_InsideTessFactor;
};


PatchTess ConstantHS(InputPatch<VertexOut, 3> patch, uint patchID : SV_PrimitiveID)
{
	PatchTess pt;

	float3 centerL = (patch[0].PosL + patch[1].PosL + patch[2].PosL) / 3.0f;
	float3 centerW = mul(float4(centerL, 1.0f), gWorld).xyz;

	float d = distance(centerW, gEyePosW);

	const float d0 = 5.0f;
	const float d1 = 20.0f;

	float tess = 10.0f * saturate((d1 - d) / (d1 - d0));

	pt.EdgeTess[0] = tess;
	pt.EdgeTess[1] = tess;
	pt.EdgeTess[2] = tess;

	pt.InsideTess[0] = tess;

	return pt;
}

struct HullOut
{
	float3 PosL : POSITION;
};

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 3> p,
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

[domain("tri")]
DomainOut DS(PatchTess patchTess,
	float3 uvw : SV_DomainLocation,
	const OutputPatch<HullOut, 3> tri)
{
	DomainOut dOut;

	float3 v = uvw.x * tri[0].PosL + uvw.y * tri[1].PosL + uvw.z * tri[2].PosL;

	v = normalize(v);

	float4 PosW = mul(float4(v, 1.0f), gWorld);

	dOut.PosH = mul(PosW, gViewProj);

	return dOut;
}

float4 PS(DomainOut pIn) : SV_Target
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}