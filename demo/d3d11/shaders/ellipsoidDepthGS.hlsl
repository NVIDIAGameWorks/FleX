#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	FluidShaderConst gParams;
};

static const float2 corners[4] =
{
	float2(0.0, 1.0), float2(0.0, 0.0), float2(1.0, 1.0), float2(1.0, 0.0)
};

[maxvertexcount(4)]
void ellipsoidDepthGS(point FluidVertexOut input[1], inout TriangleStream<FluidGeoOut> triStream)
{
	float4 gl_Position;
	float4 gl_TexCoord[6];

	float4 gl_PositionIn[1];
	float4 gl_TexCoordIn[1][6];

	gl_PositionIn[0]  = input[0].position;
	{
		[unroll]
		for (int i = 0; i < 6; i++)
			gl_TexCoordIn[0][i] = input[0].texCoord[i];
	}

	float3 pos = gl_PositionIn[0].xyz;
	float4 bounds = gl_TexCoordIn[0][0];
	float4 ndcPos = gl_TexCoordIn[0][5];

	if (ndcPos.w < 0.0)
		return;

	// frustrum culling
	const float ndcBound = 1.0;
	if (ndcPos.x < -ndcBound) return;
	if (ndcPos.x > ndcBound) return;
	if (ndcPos.y < -ndcBound) return;
	if (ndcPos.y > ndcBound) return;

	float xmin = bounds.x;
	float xmax = bounds.y;
	float ymin = bounds.z;
	float ymax = bounds.w;


	// inv quadric transform
	gl_TexCoord[0] = gl_TexCoordIn[0][1];
	gl_TexCoord[1] = gl_TexCoordIn[0][2];
	gl_TexCoord[2] = gl_TexCoordIn[0][3];
	gl_TexCoord[3] = gl_TexCoordIn[0][4];

	FluidGeoOut output;

	gl_Position = float4(xmin, ymax, 0.5, 1.0);
	output.position = gl_Position;
	output.texCoord[0] = gl_TexCoord[0];
	output.texCoord[1] = gl_TexCoord[1];
	output.texCoord[2] = gl_TexCoord[2];
	output.texCoord[3] = gl_TexCoord[3];
	triStream.Append(output);

	gl_Position = float4(xmin, ymin, 0.5, 1.0);
	output.position = gl_Position;
	output.texCoord[0] = gl_TexCoord[0];
	output.texCoord[1] = gl_TexCoord[1];
	output.texCoord[2] = gl_TexCoord[2];
	output.texCoord[3] = gl_TexCoord[3];
	triStream.Append(output);

	gl_Position = float4(xmax, ymax, 0.5, 1.0);
	output.position = gl_Position;
	output.texCoord[0] = gl_TexCoord[0];
	output.texCoord[1] = gl_TexCoord[1];
	output.texCoord[2] = gl_TexCoord[2];
	output.texCoord[3] = gl_TexCoord[3];
	triStream.Append(output);

	gl_Position = float4(xmax, ymin, 0.5, 1.0);
	output.position = gl_Position;
	output.texCoord[0] = gl_TexCoord[0];
	output.texCoord[1] = gl_TexCoord[1];
	output.texCoord[2] = gl_TexCoord[2];
	output.texCoord[3] = gl_TexCoord[3];
	triStream.Append(output);

	/*
	void main()
	{
		vec3 pos = gl_PositionIn[0].xyz;
		vec4 bounds = gl_TexCoordIn[0][0];
		vec4 ndcPos = gl_TexCoordIn[0][5];

		// frustrum culling
		const float ndcBound = 1.0;
		if (ndcPos.x < -ndcBound) return;
		if (ndcPos.x > ndcBound) return;
		if (ndcPos.y < -ndcBound) return;
		if (ndcPos.y > ndcBound) return;

		float xmin = bounds.x;
		float xmax = bounds.y;
		float ymin = bounds.z;
		float ymax = bounds.w;

		// inv quadric transform
		gl_TexCoord[0] = gl_TexCoordIn[0][1];
		gl_TexCoord[1] = gl_TexCoordIn[0][2];
		gl_TexCoord[2] = gl_TexCoordIn[0][3];
		gl_TexCoord[3] = gl_TexCoordIn[0][4];

		gl_Position = vec4(xmin, ymax, 0.0, 1.0);
		EmitVertex();

		gl_Position = vec4(xmin, ymin, 0.0, 1.0);
		EmitVertex();

		gl_Position = vec4(xmax, ymax, 0.0, 1.0);
		EmitVertex();

		gl_Position = vec4(xmax, ymin, 0.0, 1.0);
		EmitVertex();
	}
	*/
}
