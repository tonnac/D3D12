cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj;
};

cbuffer cbTime : register(b1)
{
	float gTime;
}

struct VertexIn
{
	float3 Pos   : POSITION;
	float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
	float4 Color : COLOR;
};

VertexOut VS(VertexIn vIn)
{
	VertexOut vOut;

	vOut.PosH = mul(float4(vIn.Pos, 1.0f), gWorldViewProj);

	vOut.Color = vIn.Color;

	return vOut;
}

float4 PS(VertexOut pIn) : SV_TARGET
{
	float4 outColor = float4(pIn.Color);
	outColor * (cos(gTime) * 0.5f + 0.5f);
	return pIn.Color;
}