#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	DiffuseShaderConst gParams;
};

DiffuseVertexOut diffuseVS(DiffuseVertexIn input)
{
	float3 worldPos = input.position.xyz;
	float4 eyePos = mul(gParams.modelView, float4(worldPos, 1.0));

	DiffuseVertexOut output;

	output.worldPos = input.position;	// lifetime in w
	output.viewPos = eyePos;
	output.viewVel = mul(gParams.modelView, float4(input.velocity.xyz, 0.0));
	output.color = gParams.color;

	// compute ndc pos for frustrum culling in GS
	float4 ndcPos = mul(gParams.modelViewProjection, float4(worldPos.xyz, 1.0));
	output.ndcPos = ndcPos / ndcPos.w;

	return output;
}
