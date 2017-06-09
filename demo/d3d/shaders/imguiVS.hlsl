
cbuffer params : register(b0)
{
	float4x4 transform;
};

struct Input
{
	float2 position : POSITION;
	float2 texCoord : TEXCOORD;
	float4 color : COLOR;
};

struct Output
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float4 color : COLOR;
};

Output imguiVS(Input input, uint instance : SV_InstanceID)
{
	Output output;

	output.position = mul(float4(input.position, 0.f, 1.f), transform);

	output.texCoord = input.texCoord.xy; // float2(input.texCoord.x, 1.f - input.texCoord.y);
	output.color = input.color;

	return output;
}