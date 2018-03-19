#include "shaderCommon.h"

#define ENABLE_SIMPLE_FLUID 1

cbuffer constBuf : register(b0)
{
	FluidShaderConst gParams;
};

Texture2D<float> depthTex : register(t0);
Texture2D<float3> sceneTex : register(t1);
Texture2D<float> shadowTex : register(t2);	// shadow map

SamplerState texSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);	// texture sample used to sample depth from shadow texture in this sample

// sample shadow map
float shadowSample(float3 worldPos, out float attenuation)
{
#if 0
	attenuation = 0.0f;
	return 0.5;
#else
	
	float4 pos = mul(gParams.lightTransform, float4(worldPos + gParams.lightDir*0.15, 1.0));
	pos /= pos.w;
	float3 uvw = (pos.xyz * float3(0.5, 0.5, 1.0)) + float3(0.5, 0.5, 0.0);

	attenuation = max(smoothstep(gParams.spotMax, gParams.spotMin, dot(pos.xy, pos.xy)), 0.05);

	// user clip
	if (uvw.x  < 0.0 || uvw.x > 1.0)
		return 1.0;
	if (uvw.y < 0.0 || uvw.y > 1.0)
		return 1.0;

	float s = 0.0;
	float radius = 0.002;

	uvw.y = 1.0f - uvw.y;

	[unroll]
	for (int i = 0; i < 8; i++)
	{
		float2 shadowTaps = gParams.shadowTaps[i].xy;
		shadowTaps.y = 1.0f - shadowTaps.y;
		s += shadowTex.SampleCmp(shadowSampler, uvw.xy + shadowTaps * radius, uvw.z);
	}

	s /= 8.0;
	return s;
#endif
}

float3 viewportToEyeSpace(float2 coord, float eyeZ)
{
	float2 clipPosToEye = gParams.clipPosToEye.xy;

	// find position at z=1 plane
	//float2 uv = (coord * 2.0 - float2(1.0, 1.0)) * clipPosToEye;
	float2 uv = float2(coord.x*2.0f-1.0f, (1.0f-coord.y)*2.0f - 1.0f)*clipPosToEye;
	
	return float3(-uv * eyeZ, eyeZ);
}

float3 srgbToLinear(float3 c) { const float v = 2.2; return pow(c, float3(v, v, v)); }
float3 linearToSrgb(float3 c) { const float v = 1.0 / 2.2; return pow(c, float3(v, v, v)); }

float sqr(float x) { return x*x; }
float cube(float x) { return x*x*x; }

float4 compositePS(PassthroughVertexOut input, out float depthOut : SV_DEPTH) : SV_TARGET
{
	const float4x4 projectionMatrix = gParams.projection;
	const float4x4 modelViewMatrix = gParams.modelView;
	const float4x4 modelViewMatrixInverse = gParams.inverseModelView;

	const float2 invTexScale = gParams.invTexScale.xy;

	const float3 lightDir = gParams.lightDir;
	const float3 lightPos = gParams.lightPos;
	const float spotMin = gParams.spotMin;
	const float spotMax = gParams.spotMax;
	const float ior = gParams.ior;
	const float4 color = gParams.color;

	// flip uv y-coordinate
	float2 uvCoord = float2(input.texCoord.x, 1.0f-input.texCoord.y);

	float eyeZ = depthTex.Sample(texSampler, uvCoord).x;

	if (eyeZ >= 0.0)
		discard;

	// reconstruct eye space pos from depth
	float3 eyePos = viewportToEyeSpace(uvCoord, eyeZ);

	// finite difference approx for normals, can't take dFdx because
	// the one-sided difference is incorrect at shape boundaries
	float3 zl = eyePos - viewportToEyeSpace(uvCoord - float2(invTexScale.x, 0.0), depthTex.Sample(texSampler, uvCoord - float2(invTexScale.x, 0.0)).x);
	float3 zr = viewportToEyeSpace(uvCoord + float2(invTexScale.x, 0.0), depthTex.Sample(texSampler, uvCoord + float2(invTexScale.x, 0.0)).x) - eyePos;
	float3 zt = viewportToEyeSpace(uvCoord + float2(0.0, invTexScale.y), depthTex.Sample(texSampler, uvCoord + float2(0.0, invTexScale.y)).x) - eyePos;
	float3 zb = eyePos - viewportToEyeSpace(uvCoord - float2(0.0, invTexScale.y), depthTex.Sample(texSampler, uvCoord - float2(0.0, invTexScale.y)).x);

	float3 dx = zl;
	float3 dy = zt;

	if (abs(zr.z) < abs(zl.z))
		dx = zr;

	if (abs(zb.z) < abs(zt.z))
		dy = zb;
	
	//float3 dx = ddx(eyePos.xyz);
	//float3 dy = -ddy(eyePos.xyz);

	float4 worldPos = mul(modelViewMatrixInverse, float4(eyePos, 1.0));

	float attenuation;
	float shadow = shadowSample(worldPos.xyz, attenuation);

	float3 l = mul(modelViewMatrix, float4(lightDir, 0.0)).xyz;
	float3 v = -normalize(eyePos);

	float3 n = -normalize(cross(dx, dy)); // sign difference from texcoord coordinate difference between OpenGL		
	float3 h = normalize(v + l);

	float3 skyColor = float3(0.1, 0.2, 0.4)*1.2;
	float3 groundColor = float3(0.1, 0.1, 0.2);

	float fresnel = 0.1 + (1.0 - 0.1)*cube(1.0 - max(dot(n, v), 0.0));

	float3 lVec = normalize(worldPos.xyz - lightPos);

	float ln = dot(l, n)*attenuation;

	float3 rEye = reflect(-v, n).xyz;
	float3 rWorld = mul(modelViewMatrixInverse, float4(rEye, 0.0)).xyz;

	float2 texScale = float2(0.75, 1.0);	// to account for backbuffer aspect ratio (todo: pass in)

	float refractScale = ior*0.025;
	float reflectScale = ior*0.1;

	// attenuate refraction near ground (hack)
	refractScale *= smoothstep(0.1, 0.4, worldPos.y);

	float2 refractCoord = uvCoord + n.xy*refractScale*texScale;

	// read thickness from refracted coordinate otherwise we get halos around objectsw
	float thickness = 0.8f;//max(texture2D(thicknessTex, refractCoord).x, 0.3);

	//vec3 transmission = exp(-(vec3(1.0)-color.xyz)*thickness);
	float3 transmission = (1.0 - (1.0 - color.xyz)*thickness*0.8)*color.w;
	float3 refract = sceneTex.Sample(texSampler, refractCoord).xyz*transmission;

	float2 sceneReflectCoord = uvCoord - rEye.xy*texScale*reflectScale / eyePos.z;
	float3 sceneReflect = sceneTex.Sample(texSampler, sceneReflectCoord).xyz*shadow;
	//vec3 planarReflect = texture2D(reflectTex, gl_TexCoord[0].xy).xyz;
	float3 planarReflect = float3(0.0, 0.0, 0.0);

	// fade out planar reflections above the ground
	//float3 reflect = lerp(planarReflect, sceneReflect, smoothstep(0.05, 0.3, worldPos.y)) + lerp(groundColor, skyColor, smoothstep(0.15, 0.25, rWorld.y)*shadow);
	float3 reflect = sceneReflect + lerp(groundColor, skyColor, smoothstep(0.15, 0.25, rWorld.y)*shadow);

	// lighting
	float3 diffuse = color.xyz * lerp(float3(0.29, 0.379, 0.59), float3(1.0, 1.0, 1.0), (ln*0.5 + 0.5)*max(shadow, 0.4))*(1.0 - color.w);
	float specular = 1.2*pow(max(dot(h, n), 0.0), 400.0);
	
	// write valid z
	float4 clipPos = mul(projectionMatrix, float4(0.0, 0.0, eyeZ, 1.0));
	clipPos.z /= clipPos.w;
	depthOut = clipPos.z;

	// visualize normals
	//return float4(n*0.5 + 0.5, 1.0);
	//return float4(fresnel, fresnel, fresnel, 0);
	//return float4(n, 1); 
	
	// Color
	return float4(diffuse + (lerp(refract, reflect, fresnel) + specular)*color.w, 1.0);
}
