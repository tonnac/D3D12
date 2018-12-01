Texture2D gBaseMap : register(t0);
Texture2D gEdgeMap : register(t1);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

static const float2 gTexCoords[6] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float2 TexC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vOut = (VertexOut)0.0f;

	vOut.TexC = gTexCoords[vid];

	vOut.PosH = float4(2.0f * vOut.TexC.x - 1.0f, 1.0f - 2.0f * vOut.TexC.y, 0.0f, 1.0f);

	return vOut;
}

float4 PS(VertexOut pIn) : SV_Target
{
	float4 c = gBaseMap.SampleLevel(gsamPointClamp, pIn.TexC, 0.0f);
	float4 e = gEdgeMap.SampleLevel(gsamPointClamp, pIn.TexC, 0.0f);

	return c * e;
}