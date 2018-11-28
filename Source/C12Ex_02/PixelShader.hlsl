cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj;
	float3 gEyePos;
	float gHeight;
	float3 pad1;
	float gLength;
};

struct VertexIn
{
	float3 Pos   : POSITION;
	float4 Color : COLOR;
};

struct VertexOut
{
	float3 PosH  : POSITION;
	float4 Color : COLOR;
};

struct GeoOut
{
	float4 PosH		: SV_POSITION;
	float4 Color	: COLOR;
};

VertexOut VS(VertexIn vIn)
{
	VertexOut vOut = (VertexOut)0.0f;
	vOut.PosH = vIn.Pos;
	vOut.Color = vIn.Color;

	return vOut;
}

void Subdivide(in VertexOut inVerts[3], out VertexOut outVerts[6])
{
	VertexOut v[3];

	v[0].PosH = (inVerts[0].PosH + inVerts[1].PosH) * 0.5f;
	v[1].PosH = (inVerts[1].PosH + inVerts[2].PosH) * 0.5f;
	v[2].PosH = (inVerts[2].PosH + inVerts[0].PosH) * 0.5f;

	v[0].PosH = normalize(v[0].PosH);
	v[1].PosH = normalize(v[1].PosH);
	v[2].PosH = normalize(v[2].PosH);

	v[0].Color = (inVerts[0].Color + inVerts[1].Color) * 0.5f;
	v[1].Color = (inVerts[1].Color + inVerts[2].Color) * 0.5f;
	v[2].Color = (inVerts[2].Color + inVerts[0].Color) * 0.5f;

	outVerts[0] = inVerts[0];
	outVerts[1] = v[0];
	outVerts[2] = v[2];
	outVerts[3] = v[1];
	outVerts[4] = inVerts[2];
	outVerts[5] = inVerts[1];
}

void OutputSubdivision(VertexOut v[6],
	inout TriangleStream<GeoOut> triStream)
{
	GeoOut gOut[6];
	for (int i = 0; i < 6; ++i)
	{
		gOut[i].PosH = mul(float4(v[i].PosH, 1.0f), gWorldViewProj);
		gOut[i].Color = v[i].Color;
	}

	for (int i = 0; i < 5; ++i)
	{
		triStream.Append(gOut[i]);
	}
	triStream.RestartStrip();
	triStream.Append(gOut[1]);
	triStream.Append(gOut[5]);
	triStream.Append(gOut[3]);
}

[maxvertexcount(32)]
void GS(triangle VertexOut gIn[3],
		inout TriangleStream<GeoOut> triStream)
{
	if (gLength < 15.0f)
	{

	}
	if (gLength < 30.0f && gLength >= 15.0f)
	{
		VertexOut v[6];
		Subdivide(gIn, v);
		OutputSubdivision(v, triStream);
	}
	else
	{
		GeoOut gOut[3];
		for (int i = 0; i < 3; ++i)
		{
			gOut[i].PosH = mul(float4(gIn[i].PosH, 1.0f), gWorldViewProj);
			gOut[i].Color = gIn[i].Color;
		}
		triStream.Append(gOut[0]);
		triStream.Append(gOut[1]);
		triStream.Append(gOut[2]);
	}
}

float4 PS(GeoOut pIn) : SV_TARGET
{
	return pIn.Color;
}