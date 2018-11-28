cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj;
	float3 pad;
	float gHeight;
};

struct VertexIn
{
	float3 Pos   : POSITION;
	float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH  : POSITION;
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
	vOut.PosH = float4(vIn.Pos, 1.0f);
	vOut.Color = vIn.Color;

	return vOut;
}

void Subdivide(in VertexOut inVerts[2], out VertexOut outVerts[6])
{
	VertexOut v[4];

	v[0].PosH = float4(inVerts[0].PosH.x, inVerts[0].PosH.y + gHeight, inVerts[0].PosH.z, inVerts[0].PosH.w);
	v[1].PosH = float4(inVerts[1].PosH.x, inVerts[1].PosH.y + gHeight, inVerts[1].PosH.z, inVerts[1].PosH.w);
	v[2].PosH = float4(inVerts[0].PosH.x, inVerts[0].PosH.y - gHeight, inVerts[0].PosH.z, inVerts[0].PosH.w);
	v[3].PosH = float4(inVerts[1].PosH.x, inVerts[1].PosH.y - gHeight, inVerts[1].PosH.z, inVerts[1].PosH.w);

	v[0].Color = inVerts[0].Color;
	v[1].Color = inVerts[1].Color;
	v[2].Color = inVerts[0].Color;
	v[3].Color = inVerts[1].Color;

	outVerts[0] = inVerts[0];
	outVerts[1] = v[0];
	outVerts[2] = inVerts[1];
	outVerts[3] = v[1];
	outVerts[4] = v[2];
	outVerts[5] = v[3];
}

void OutputSubdivision(VertexOut v[6],
	inout TriangleStream<GeoOut> triStream)
{
	GeoOut gOut[6];
	for (int i = 0; i < 6; ++i)
	{
		gOut[i].PosH = mul(float4(v[i].PosH), gWorldViewProj);
		gOut[i].Color = v[i].Color;
	}

	triStream.Append(gOut[0]);
	triStream.Append(gOut[1]);
	triStream.Append(gOut[2]);

	triStream.Append(gOut[1]);
	triStream.Append(gOut[3]);
	triStream.Append(gOut[2]);

	triStream.Append(gOut[0]);
	triStream.Append(gOut[5]);
	triStream.Append(gOut[4]);

	triStream.Append(gOut[5]);
	triStream.Append(gOut[0]);
	triStream.Append(gOut[2]);

	triStream.RestartStrip();
}

[maxvertexcount(12)]
void GS(line VertexOut gIn[2],
		inout TriangleStream<GeoOut> triStream)
{
	VertexOut v[6];
	Subdivide(gIn, v);
	OutputSubdivision(v, triStream);
}

float4 PS(GeoOut pIn) : SV_TARGET
{
	return pIn.Color;
}