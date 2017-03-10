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


float4 ellipsoidDepthPS(FluidGeoOut input
	, out float gl_FragDepth : SV_DEPTH
) : SV_TARGET
{
	const float4x4 gl_ProjectionMatrix = gParams.projection;
	const float4x4 gl_ProjectionMatrixInverse = gParams.projection_inverse;
	const float3 invViewport = gParams.invViewport;

	float4 gl_FragColor;
	float4 gl_FragCoord;
	float4 gl_TexCoord[6];

	gl_FragCoord = input.position;
	[unroll]
	for (int i = 0; i < 4; i++)
		gl_TexCoord[i] = input.texCoord[i];

	// transform from view space to parameter space
	//column_major
	float4x4 invQuadric;
	invQuadric._m00_m10_m20_m30 = gl_TexCoord[0];
	invQuadric._m01_m11_m21_m31 = gl_TexCoord[1];
	invQuadric._m02_m12_m22_m32 = gl_TexCoord[2];
	invQuadric._m03_m13_m23_m33 = gl_TexCoord[3];

	//float4 ndcPos = float4(gl_FragCoord.xy * invViewport.xy * float2(2.0, 2.0) - float2(1.0, 1.0), -1.0, 1.0);
	float4 ndcPos = float4(gl_FragCoord.x*invViewport.x*2.0f-1.0f, (1.0f-gl_FragCoord.y*invViewport.y)*2.0 - 1.0, 0.0f, 1.0);
	float4 viewDir = mul(gl_ProjectionMatrixInverse, ndcPos);

	// ray to parameter space
	float4 dir = mul(invQuadric, float4(viewDir.xyz, 0.0));
	float4 origin = invQuadric._m03_m13_m23_m33;

	// set up quadratric equation
	float a = sqr(dir.x) + sqr(dir.y) + sqr(dir.z);
	float b = dir.x*origin.x + dir.y*origin.y + dir.z*origin.z - dir.w*origin.w;
	float c = sqr(origin.x) + sqr(origin.y) + sqr(origin.z) - sqr(origin.w);

	float minT;
	float maxT;

	if (solveQuadratic(a, 2.0 * b, c, minT, maxT))
	{
		float3 eyePos = viewDir.xyz*minT;
		float4 ndcPos = mul(gl_ProjectionMatrix, float4(eyePos, 1.0));
		ndcPos.z /= ndcPos.w;

		gl_FragColor = float4(eyePos.z, 1.0, 1.0, 1.0);
		gl_FragDepth = ndcPos.z;	

		return gl_FragColor;
	}
	
	// kill pixels outside of ellipsoid
	discard;


	gl_FragColor = 0.0f;
	gl_FragDepth = 1.0f;
	
	return gl_FragColor;
}
