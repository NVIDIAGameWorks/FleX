#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	PointShaderConst gParams;
};

PointVertexOut pointVS(PointVertexIn input, uint instance : SV_VertexID)
{
	const float4 gl_Vertex = input.position;
	const float4x4 gl_ModelViewMatrix = gParams.modelview;

	float density = input.density;
	int phase = input.phase;

	// calculate window-space point size
	float4 viewPos = mul(gl_ModelViewMatrix, float4(gl_Vertex.xyz, 1.0));

	PointVertexOut output;
	output.position = viewPos;
	output.density = density;
	output.phase = phase;
	output.vertex = gl_Vertex;

	return output;
}
