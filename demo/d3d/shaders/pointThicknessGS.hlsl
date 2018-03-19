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
void pointThicknessGS(point FluidVertexOut input[1], inout TriangleStream<FluidGeoOut> triStream)
{
	float4 gl_Position;
	float4 gl_TexCoord[4];

	{
		[unroll]
		for (int i = 0; i < 4; i++)
			gl_TexCoord[i] = float4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	const float4x4 gl_ModelViewMatrix = gParams.modelView;
	const float pointRadius = gParams.pointRadius;
	//const float pointScale = gParams.pointScale;

	float4 viewPos = input[0].position;
	float4 gl_Vertex = input[0].bounds;	// retrieve gl_Vertex from bounds

	//float spriteSize = pointScale * (pointRadius / gl_Position.w);
	float spriteSize = pointRadius * 2;

	FluidGeoOut output;

	for (int i = 0; i < 4; ++i)
	{

		float4 eyePos = viewPos;									// start with point position
		eyePos.xy += spriteSize * (corners[i] - float2(0.5, 0.5));	// add corner position
		gl_Position = mul(gParams.projection, eyePos);				// complete transformation

		gl_TexCoord[0].xy = corners[i].xy;							// use corner as texCoord
		gl_TexCoord[0].y = 1.0f - gl_TexCoord[0].y;					// flip the y component of uv (glsl to hlsl conversion)
		//gl_TexCoord[1] = mul(gl_ModelViewMatrix, float4(gl_Vertex.xyz, 1.0));

		output.position = gl_Position;
		//[unroll]
		//for (int j = 0; j < 4; j++)
		//	output.texCoord[j] = gl_TexCoord[j];
		output.invQ0 = gl_TexCoord[0];
		output.invQ1 = gl_TexCoord[1];
		output.invQ2 = gl_TexCoord[2];
		output.invQ3 = gl_TexCoord[3];

		triStream.Append(output);
	}
}
