#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	PointShaderConst gParams;
};

Texture2D<float> shadowTexture : register(t0);	// shadow map

SamplerComparisonState shadowSampler : register(s0);	// texture sample used to sample depth from shadow texture in this sample

float sqr(float x) { return x * x; }

float shadowSample(float4 lightOffsetPosition)
{
	float3 pos = float3(lightOffsetPosition.xyz / lightOffsetPosition.w);
	//float3 uvw = (pos.xyz * 0.5) + vec3(0.5);
	float3 uvw = (pos.xyz * float3(0.5, 0.5, 1.0)) + float3(0.5, 0.5, 0.0);
	
	// user clip
	if (uvw.x  < 0.0 || uvw.x > 1.0)
		return 1.0;
	if (uvw.y < 0.0 || uvw.y > 1.0)
		return 1.0;

	float s = 0.0;
	float radius = 0.002;

	// flip uv y-coordinate
	uvw.y = 1.0f - uvw.y;

	[unroll]
	for (int i = 0; i < 8; i++)
	{
		float2 shadowTaps = gParams.shadowTaps[i].xy;
		shadowTaps.y = 1.0f - shadowTaps.y;

		//s += shadow2D(shadowTex, vec3(uvw.xy + shadowTaps[i] * radius, uvw.z)).r;
		s += shadowTexture.SampleCmp(shadowSampler, uvw.xy + shadowTaps * radius, uvw.z);
	}
	s /= 8.0;

	return s;
}

float4 pointPS(PointGeoOut input) : SV_TARGET
{
	const float spotMin = gParams.spotMin;
	const float spotMax = gParams.spotMax;

	// calculate normal from texture coordinates
	float3 normal;
	normal.xy = input.texCoord * float2(2.0, -2.0) + float2(-1.0, 1.0);
	float mag = dot(normal.xy, normal.xy);
	if (mag > 1.0) 
	{
		discard;   // kill pixels outside circle
	}

	normal.z = sqrt(1.0 - mag);

	if (gParams.mode == 2)
	{
		float alpha = normal.z * input.reflectance.w;
		return float4(input.reflectance.xyz * alpha, alpha);
	}

	// calculate lighting
	float shadow = shadowSample(input.lightOffsetPosition);

	float3 lPos = float3(input.lightOffsetPosition.xyz / input.lightOffsetPosition.w);
	float attenuation = max(smoothstep(spotMax, spotMin, dot(lPos.xy, lPos.xy)), 0.05);

	float3 diffuse = float3(0.9, 0.9, 0.9);
	float3 reflectance = input.reflectance.xyz;

	float3 lo = diffuse * reflectance * max(0.0, sqr(-dot(input.viewLightDir, normal) * 0.5 + 0.5)) * max(0.2, shadow) * attenuation;

	const float tmp = 1.0 / 2.2;
	return float4(pow(abs(lo), float3(tmp, tmp, tmp)), 1.0);
}
