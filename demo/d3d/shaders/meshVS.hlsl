#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	MeshShaderConst gParams;
};

MeshVertexOut meshVS(MeshVertexIn input)
{
	const float4x4 modelViewProjectionMatrix = gParams.modelViewProjection;
	const float4x4 modelViewMatrix = gParams.modelView;
	const float4x4 objectTransform  = gParams.objectTransform;
	const float4x4 lightTransform = gParams.lightTransform;
	
	float3 n = normalize(mul(objectTransform, float4(input.normal, 0.0)).xyz);
	float3 p = mul(objectTransform, float4(input.position.xyz, 1.0)).xyz;

	// calculate window-space point size
	MeshVertexOut output;
	output.position = mul(modelViewProjectionMatrix, float4(p + gParams.expand * n, 1.0));

	output.worldNormal = n;
	output.lightOffsetPosition = mul(lightTransform, float4(p + n * gParams.bias, 1.0));
	output.viewLightDir = mul(modelViewMatrix, float4(gParams.lightDir, 0.0));
	output.worldPosition = p;
	if (gParams.colorArray)
		output.color = input.color;
	else
		output.color = gParams.color;
	output.texCoord = float2(input.texCoord.x, 1.0f - input.texCoord.y);	// flip the y component of uv (glsl to hlsl conversion)
	output.secondaryColor = gParams.secondaryColor;
	output.viewPosition = mul(modelViewMatrix, float4(input.position.xyz, 1.0));

	return output;
}
