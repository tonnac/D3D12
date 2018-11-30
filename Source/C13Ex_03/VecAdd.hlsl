struct Data
{
	float3 p;
};

struct Output
{
	float v;
};

ConsumeStructuredBuffer<Data> gInputA : register(u0);
AppendStructuredBuffer<Output> gOutput : register(u1);


[numthreads(32, 1, 1)]
void CS()
{
	Data d = gInputA.Consume();
	Output e;
	e.v = length(d.p);
	gOutput.Append(e);
}