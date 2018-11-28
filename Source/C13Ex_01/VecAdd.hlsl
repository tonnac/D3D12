
struct Data
{
	float3 v1;
};

struct OutData
{
	float v;
};

StructuredBuffer<Data> gInputA : register(t0);
RWStructuredBuffer<float> gOutput : register(u0);


[numthreads(32, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
	gOutput[DTid.x] = length(float3(gInputA[DTid.x].v1));
}