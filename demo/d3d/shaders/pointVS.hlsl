#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	PointShaderConst gParams;
};

PointVertexOut pointVS(PointVertexIn input, uint instance : SV_VertexID)
{
	const float4 modelPosition = input.position;
	const float4x4 modelViewMatrix = gParams.modelView;

	float density = input.density;
	int phase = input.phase;

	// calculate window-space point size
	float4 viewPos = mul(modelViewMatrix, float4(modelPosition.xyz, 1.0));

	PointVertexOut output;
	output.viewPosition = viewPos;
	output.density = density;
	output.phase = phase;
	output.modelPosition = modelPosition;
	return output;
}
