#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	MeshShaderConst gParams;
};

Texture2D<float> shadowTexture : register(t0);	// shadow map
SamplerComparisonState shadowSampler : register(s0);	// texture sample used to sample depth from shadow texture in this sample

// sample shadow map
float shadowSample(float4 lightOffsetPosition)
{
	float3 pos = float3(lightOffsetPosition.xyz / lightOffsetPosition.w);
	float3 uvw = (pos.xyz * float3(0.5, 0.5, 1.0)) + float3(0.5, 0.5, 0.0);

	// user clip
	if (uvw.x  < 0.0 || uvw.x > 1.0)
		return 1.0;
	if (uvw.y < 0.0 || uvw.y > 1.0)
		return 1.0;

	float s = 0.0;
	float radius = 0.002;

	const int numTaps = 12;

	// flip uv y-coordinate
	uvw.y = 1.0f - uvw.y;

	[unroll]
	for (int i = 0; i < numTaps; i++)
	{
		float2 shadowTaps = gParams.shadowTaps[i].xy;
		shadowTaps.y = 1.0f - shadowTaps.y;
		s += shadowTexture.SampleCmp(shadowSampler, uvw.xy + shadowTaps * radius, uvw.z);
	}
	s /= numTaps;

	return s;
}

float filterwidth(float2 v)
{
	float2 fw = max(abs(ddx(v)), abs(ddy(v)));
	return max(fw.x, fw.y);
}

float2 bump(float2 x)
{
	return (floor((x) / 2) + 2.f * max(((x) / 2) - floor((x) / 2) - .5f, 0.f));
}

float checker(float2 uv)
{
	float width = filterwidth(uv);
	float2 p0 = uv - 0.5 * width;
	float2 p1 = uv + 0.5 * width;

	float2 i = (bump(p1) - bump(p0)) / width;
	return i.x * i.y + (1 - i.x) * (1 - i.y);
}

float4 meshPS(MeshVertexOut input, bool isFrontFace : SV_IsFrontFace) : SV_TARGET
{
	const float4 fogColor = gParams.fogColor;
	const float3 lightDir = gParams.lightDir;
	const float3 lightPos = gParams.lightPos;
	const float spotMin = gParams.spotMin;
	const float spotMax = gParams.spotMax;
	const int grid = gParams.grid;
	const int tex = gParams.tex;

	// calculate lighting
	float shadow = max(shadowSample(input.lightOffsetPosition), 0.5);
	//float shadow = 0.5f;

	float3 lVec = normalize(input.worldPosition - lightPos);
	float3 lPos = float3(input.lightOffsetPosition.xyz / input.lightOffsetPosition.w);
	float attenuation = max(smoothstep(spotMax, spotMin, dot(lPos.xy, lPos.xy)), 0.05);
	
	float3 n = input.worldNormal;
	float3 color = input.color.xyz;

	if (!isFrontFace)
	{
		color = input.secondaryColor.xyz;
		n *= -1.0f;
	}

	if (grid && (n.y > 0.995))
	{
		color *= 1.0 - 0.25 * checker(float2(input.worldPosition.x, input.worldPosition.z));
	}
	else if (grid && abs(n.z) > 0.995)
	{
		color *= 1.0 - 0.25 * checker(float2(input.worldPosition.y, input.worldPosition.x));
	}

	if (tex)
	{
		//color = texture2D(tex, gl_TexCoord[5].xy).xyz;
	}
	
	// direct light term
	float wrap = 0.0;
	float3 diffuse = color * float3(1.0, 1.0, 1.0) * max(0.0, (-dot(lightDir, n) + wrap) / (1.0 + wrap) * shadow) * attenuation;

	// wrap ambient term aligned with light dir
	float3 light = float3(0.03, 0.025, 0.025) * 1.5;
	float3 dark = float3(0.025, 0.025, 0.03);
	//float3 ambient = 4.0 * color * lerp(dark, light, -dot(lightDir, n) * 0.5 + 0.5) * attenuation;
	float3 ambient = 4.0 * color * lerp(dark, light, -dot(lightDir, n) * float3(0.5, 0.5, 1.0) + float3(0.5, 0.5, 0.0)) * attenuation;
	
	float3 fog = lerp(fogColor.xyz, diffuse + ambient, exp(input.viewPosition.z * fogColor.w));
	
	//outColor = float4(pow(fog, float3(1.0 / 2.2)), 1.0);
	const float tmp = 1.0 / 2.2;

	float4 outColor = float4(pow(abs(fog), float3(tmp, tmp, tmp)), 1.0);
	
	return outColor;
}
