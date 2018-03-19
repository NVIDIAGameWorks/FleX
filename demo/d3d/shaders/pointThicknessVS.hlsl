#include "shaderCommon.h"

#pragma warning (disable : 3578)

cbuffer constBuf : register(b0)
{
	FluidShaderConst gParams;
};

FluidVertexOut pointThicknessVS(FluidVertexIn input, uint instance : SV_VertexID)
{
	const float4 gl_Vertex = input.position;
	const float4x4 gl_ModelViewMatrix = gParams.modelView;

	// calculate window-space point size
	float4 viewPos = mul(gl_ModelViewMatrix, float4(gl_Vertex.xyz, 1.0));

	FluidVertexOut output;
	output.position = viewPos;
	output.bounds = gl_Vertex;	// store gl_Vertex in bounds

	return output;
}
