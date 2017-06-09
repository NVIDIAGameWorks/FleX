
cbuffer params : register(b0)
{
	float4x4  projectionViewWorld;
};

struct Input
{
	float3 position : POSITION;
	float4 color : COLOR;
};

struct Output
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

Output debugLineVS(Input input)
{
	Output output;
	output.position = mul(projectionViewWorld, float4(input.position, 1.0f));
	output.color = input.color;

	return output;
}
