#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	PointShaderConst gParams;
};

Texture2D<float> shadowTexture : register(t0);	// shadow map

SamplerComparisonState shadowSampler : register(s0);	// texture sample used to sample depth from shadow texture in this sample

float sqr(float x) { return x * x; }

float shadowSample(float4 gl_TexCoord[6])
{
	float3 pos = float3(gl_TexCoord[1].xyz / gl_TexCoord[1].w);
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
		s += shadowTexture.SampleCmpLevelZero(shadowSampler, uvw.xy + shadowTaps * radius, uvw.z);
	}
	s /= 8.0;

	return s;
}

float4 pointPS(PointGeoOut input
	//, out float gl_FragDepth : SV_DEPTH
) : SV_TARGET
{
	//gl_FragDepth = 0.0f;

	const float spotMin = gParams.spotMin;
	const float spotMax = gParams.spotMax;

	float4 gl_FragColor;
	float4 gl_TexCoord[6];

	[unroll]
	for (int i = 0; i < 6; i++)
		gl_TexCoord[i] = input.texCoord[i];

	// calculate normal from texture coordinates
	float3 normal;
	normal.xy = gl_TexCoord[0].xy * float2(2.0, -2.0) + float2(-1.0, 1.0);
	float mag = dot(normal.xy, normal.xy);
	if (mag > 1.0) discard;   // kill pixels outside circle
	normal.z = sqrt(1.0 - mag);

	if (gParams.mode == 2)
	{
		float alpha = normal.z * gl_TexCoord[3].w;
		gl_FragColor.xyz = gl_TexCoord[3].xyz * alpha;
		gl_FragColor.w = alpha;

		return gl_FragColor;
	}

	// calculate lighting
	float shadow = shadowSample(gl_TexCoord);

	float3 lPos = float3(gl_TexCoord[1].xyz / gl_TexCoord[1].w);
	float attenuation = max(smoothstep(spotMax, spotMin, dot(lPos.xy, lPos.xy)), 0.05);

	float3 diffuse = float3(0.9, 0.9, 0.9);
	float3 reflectance = gl_TexCoord[3].xyz;

	float3 Lo = diffuse * reflectance * max(0.0, sqr(-dot(gl_TexCoord[2].xyz, normal) * 0.5 + 0.5)) * max(0.2, shadow) * attenuation;

	const float tmp = 1.0 / 2.2;
	gl_FragColor = float4(pow(abs(Lo), float3(tmp, tmp, tmp)), 1.0);

	/*
	const float pointRadius = gParams.pointRadius;
	const float4x4 gl_ProjectionMatrix = gParams.projection;

	float3 eyePos = gl_TexCoord[5].xyz + normal * pointRadius;
	float4 ndcPos = mul(gl_ProjectionMatrix, float4(eyePos, 1.0));
	ndcPos.z /= ndcPos.w;
	gl_FragDepth = ndcPos.z;
	*/

	return gl_FragColor;
}
