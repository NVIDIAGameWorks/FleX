
struct Input
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float4 color : COLOR;
};

Texture2D<float> tex : register(t0);
SamplerState texSampler : register(s0);

float4 imguiPS(Input input) : SV_TARGET
{
	float4 color = input.color;

	if (input.texCoord.x >= 0.f)
	{
		color.a *= tex.SampleLevel(texSampler, input.texCoord, 0.f);
	}

	return color;
}