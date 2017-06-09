#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	PointShaderConst gParams;
};

static const float2 corners[4] =
{
	float2(0.0, 1.0), float2(0.0, 0.0), float2(1.0, 1.0), float2(1.0, 0.0)
};

[maxvertexcount(4)]
void pointGS(point PointVertexOut input[1], inout TriangleStream<PointGeoOut> triStream)
{
	const float4x4 modelViewMatrix = gParams.modelView;
	const float pointRadius = gParams.pointRadius;
	const float pointScale = gParams.pointScale;
	const float4x4 lightTransform = gParams.lightTransform;
	const float3 lightDir = gParams.lightDir.xyz;
	const int mode = gParams.mode;

	const float4 viewPosition = input[0].viewPosition;
	const float density = input[0].density;
	const unsigned int phase = input[0].phase;
	const float4 modelPosition = input[0].modelPosition;

	//float spriteSize = (pointRadius / viewPos.z);
	const float spriteSize = pointRadius * 2;

	PointGeoOut output;
	for (int i = 0; i < 4; ++i)
	{
		const float2 corner = corners[i];
		float4 eyePos = viewPosition;									// start with point position
		eyePos.xy += spriteSize * (corner - float2(0.5, 0.5));		// add corner position
		output.position = mul(gParams.projection, eyePos);			// complete transformation

		// use corner as texCoord,  flip the y component of uv (glsl to hlsl conversion)
		output.texCoord = float2(corner.x, 1.0f - corner.y);
		output.lightOffsetPosition = mul(lightTransform, float4(modelPosition.xyz - lightDir * pointRadius * 2.0, 1.0));
		output.viewLightDir = mul(modelViewMatrix, float4(lightDir, 0.0)).xyz;

		if (mode == 1)
		{
			// density visualization
			if (density < 0.0f)
				output.reflectance = float4(lerp(float3(0.1, 0.1, 1.0), float3(0.1, 1.0, 1.0), -density), 0);
			else
				output.reflectance = float4(lerp(float3(1.0, 1.0, 1.0), float3(0.1, 0.2, 1.0), density), 0);
		}
		else if (mode == 2)
		{
			//gl_PointSize *= clamp(inPosition.w * 0.25, 0.0f, 1.0);
			float tmp = clamp(modelPosition.w * 0.05, 0.0f, 1.0);
			output.reflectance = float4(tmp, tmp, tmp, tmp);
		}
		else
		{
			output.reflectance = float4(lerp(gParams.colors[phase % 8].xyz * 2.0, float3(1.0, 1.0, 1.0), 0.1), 0);
		}

		output.modelPosition= modelPosition.xyz;
		output.viewPosition = viewPosition.xyz;

		triStream.Append(output);
	}
}
