#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	FluidShaderConst gParams;
};

// returns 1.0 for x==0.0 (unlike glsl)
float Sign(float x) { return x < 0.0 ? -1.0 : 1.0; }

bool solveQuadratic(float a, float b, float c, out float minT, out float maxT)
{
#if 0
	// for debugging
	minT = -0.5;
	maxT = 0.5;
	return true;
#else
	//minT = 0.0f;
	//maxT = 0.0f;
#endif

	if (a == 0.0 && b == 0.0)
	{
		minT = maxT = 0.0;
		return false;
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

float DotInvW(float4 a, float4 b) { return a.x*b.x + a.y*b.y + a.z*b.z - a.w*b.w; }

FluidVertexOut ellipsoidDepthVS(FluidVertexIn input, uint instance : SV_VertexID)
{
	float4 gl_Position;
	float4 gl_TexCoord[6];

	const float4 gl_Vertex = input.position;
	const float4 q1 = input.q1;
	const float4 q2 = input.q2;
	const float4 q3 = input.q3;

	const float4x4 gl_ModelViewProjectionMatrix = gParams.modelviewprojection;
	const float4x4 gl_ModelViewMatrixInverse = gParams.modelview_inverse;

	float3 worldPos = gl_Vertex.xyz;
	
	// construct quadric matrix
	float4x4 q;
	q._m00_m10_m20_m30 = float4(q1.xyz*q1.w, 0.0);
	q._m01_m11_m21_m31 = float4(q2.xyz*q2.w, 0.0);
	q._m02_m12_m22_m32 = float4(q3.xyz*q3.w, 0.0);
	q._m03_m13_m23_m33 = float4(worldPos, 1.0);

	// transforms a normal to parameter space (inverse transpose of (q*modelview)^-T)
	float4x4 invClip = /*transpose*/(mul(gl_ModelViewProjectionMatrix, q));

	// solve for the right hand bounds in homogenous clip space
	float a1 = DotInvW(invClip[3], invClip[3]);
	float b1 = -2.0f*DotInvW(invClip[0], invClip[3]);
	float c1 = DotInvW(invClip[0], invClip[0]);

	float xmin;
	float xmax;
	solveQuadratic(a1, b1, c1, xmin, xmax);

	// solve for the right hand bounds in homogenous clip space
	float a2 = DotInvW(invClip[3], invClip[3]);
	float b2 = -2.0f*DotInvW(invClip[1], invClip[3]);
	float c2 = DotInvW(invClip[1], invClip[1]);

	float ymin;
	float ymax;
	solveQuadratic(a2, b2, c2, ymin, ymax);

	gl_Position = float4(worldPos.xyz, 1.0);
	gl_TexCoord[0] = float4(xmin, xmax, ymin, ymax);

	// construct inverse quadric matrix (used for ray-casting in parameter space)
	float4x4 invq;
	invq._m00_m10_m20_m30 = float4(q1.xyz / q1.w, 0.0);
	invq._m01_m11_m21_m31 = float4(q2.xyz / q2.w, 0.0);
	invq._m02_m12_m22_m32 = float4(q3.xyz / q3.w, 0.0);
	invq._m03_m13_m23_m33 = float4(0.0, 0.0, 0.0, 1.0);

	invq = transpose(invq);
	invq._m03_m13_m23_m33 = -(mul(invq, gl_Position));

	// transform a point from view space to parameter space
	invq = mul(invq, gl_ModelViewMatrixInverse);

	// pass down
	gl_TexCoord[1] = invq._m00_m10_m20_m30;
	gl_TexCoord[2] = invq._m01_m11_m21_m31;
	gl_TexCoord[3] = invq._m02_m12_m22_m32;
	gl_TexCoord[4] = invq._m03_m13_m23_m33;

	// compute ndc pos for frustrum culling in GS
	float4 ndcPos = mul(gl_ModelViewProjectionMatrix, float4(worldPos.xyz, 1.0));
	gl_TexCoord[5].xyz = ndcPos.xyz / ndcPos.w;
	gl_TexCoord[5].w = ndcPos.w;

	FluidVertexOut output;
	output.position = gl_Position;
	[unroll]
	for (int j = 0; j < 6; j++)
		output.texCoord[j] = gl_TexCoord[j];

	return output;

	/*
	// rotation matrix in xyz, scale in w
	attribute vec4 q1;
	attribute vec4 q2;
	attribute vec4 q3;

	void main()
	{
		vec3 worldPos = gl_Vertex.xyz;// - vec3(0.0, 0.1*0.25, 0.0);	// hack move towards ground to account for anisotropy

		// construct quadric matrix
		mat4 q;
		q[0] = vec4(q1.xyz*q1.w, 0.0);
		q[1] = vec4(q2.xyz*q2.w, 0.0);
		q[2] = vec4(q3.xyz*q3.w, 0.0);
		q[3] = vec4(worldPos, 1.0);

		// transforms a normal to parameter space (inverse transpose of (q*modelview)^-T)
		mat4 invClip = transpose(gl_ModelViewProjectionMatrix*q);

		// solve for the right hand bounds in homogenous clip space
		float a1 = DotInvW(invClip[3], invClip[3]);
		float b1 = -2.0f*DotInvW(invClip[0], invClip[3]);
		float c1 = DotInvW(invClip[0], invClip[0]);

		float xmin;
		float xmax;
		solveQuadratic(a1, b1, c1, xmin, xmax);

		// solve for the right hand bounds in homogenous clip space
		float a2 = DotInvW(invClip[3], invClip[3]);
		float b2 = -2.0f*DotInvW(invClip[1], invClip[3]);
		float c2 = DotInvW(invClip[1], invClip[1]);

		float ymin;
		float ymax;
		solveQuadratic(a2, b2, c2, ymin, ymax);

		gl_Position = vec4(worldPos.xyz, 1.0);
		gl_TexCoord[0] = vec4(xmin, xmax, ymin, ymax);

		// construct inverse quadric matrix (used for ray-casting in parameter space)
		mat4 invq;
		invq[0] = vec4(q1.xyz / q1.w, 0.0);
		invq[1] = vec4(q2.xyz / q2.w, 0.0);
		invq[2] = vec4(q3.xyz / q3.w, 0.0);
		invq[3] = vec4(0.0, 0.0, 0.0, 1.0);

		invq = transpose(invq);
		invq[3] = -(invq*gl_Position);

		// transform a point from view space to parameter space
		invq = invq*gl_ModelViewMatrixInverse;

		// pass down
		gl_TexCoord[1] = invq[0];
		gl_TexCoord[2] = invq[1];
		gl_TexCoord[3] = invq[2];
		gl_TexCoord[4] = invq[3];

		// compute ndc pos for frustrum culling in GS
		vec4 ndcPos = gl_ModelViewProjectionMatrix * vec4(worldPos.xyz, 1.0);
		gl_TexCoord[5] = ndcPos / ndcPos.w;
	}
	*/
}
