#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	FluidShaderConst gParams;
};

float Sign(float x) { return x < 0.0 ? -1.0 : 1.0; }

bool solveQuadratic(float a, float b, float c, out float minT, out float maxT)
{
#if 0
	minT = 0.0f;
	maxT = 0.0f;
#endif

	if (a == 0.0 && b == 0.0)
	{
		minT = maxT = 0.0;
		return true;
	}

	float discriminant = b*b - 4.0*a*c;

	if (discriminant < 0.0)
	{
		return false;
	}

	float t = -0.5*(b + Sign(b)*sqrt(discriminant));
	minT = t / a;
	maxT = c / t;

	if (minT > maxT)
	{
		float tmp = minT;
		minT = maxT;
		maxT = tmp;
	}

	return true;
}

float sqr(float x) { return x * x; }

float ellipsoidDepthPS(FluidGeoOut input , out float depthOut : SV_DEPTH) : SV_TARGET
{
	const float4x4 projectionMatrix = gParams.projection;
	const float4x4 projectionMatrixInverse = gParams.inverseProjection;
	const float3 invViewport = gParams.invViewport;
	const float4 position = input.position;

	// transform from view space to parameter space
	//column_major
	float4x4 invQuadric;
	invQuadric._m00_m10_m20_m30 = input.invQ0;
	invQuadric._m01_m11_m21_m31 = input.invQ1;
	invQuadric._m02_m12_m22_m32 = input.invQ2;
	invQuadric._m03_m13_m23_m33 = input.invQ3;

	float4 ndcPos = float4(position.x*invViewport.x*2.0f-1.0f, (1.0f-position.y*invViewport.y)*2.0 - 1.0, 0.0f, 1.0);
	float4 viewDir = mul(projectionMatrixInverse, ndcPos);

	// ray to parameter space
	float4 dir = mul(invQuadric, float4(viewDir.xyz, 0.0));
	float4 origin = invQuadric._m03_m13_m23_m33;

	// set up quadratric equation
	float a = sqr(dir.x) + sqr(dir.y) + sqr(dir.z);
	float b = dir.x*origin.x + dir.y*origin.y + dir.z*origin.z - dir.w*origin.w;
	float c = sqr(origin.x) + sqr(origin.y) + sqr(origin.z) - sqr(origin.w);

	float minT;
	float maxT;

	if (!solveQuadratic(a, 2.0 * b, c, minT, maxT))
	{
		discard;
	}
	{
		float3 eyePos = viewDir.xyz*minT;
		float4 ndcPos = mul(projectionMatrix, float4(eyePos, 1.0));
		ndcPos.z /= ndcPos.w;

		depthOut = ndcPos.z;	
		return eyePos.z;
	}
}
