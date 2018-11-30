StructuredBuffer<float3> gInputA : register(t0);
RWStructuredBuffer<float> gOutput : register(u0);


[numthreads(32, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
	gOutput[DTid.x] = length(gInputA[DTid.x].xyz);
}