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
	float4 gl_Position;
	float4 gl_TexCoord[6];

	{
		[unroll]
		for (int i = 0; i < 6; i++)
			gl_TexCoord[i] = float4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	const float4x4 gl_ModelViewMatrix = gParams.modelview;
	const float pointRadius = gParams.pointRadius;
	const float pointScale = gParams.pointScale;
	const float4x4 lightTransform = gParams.lightTransform;
	const float3 lightDir = gParams.lightDir.xyz;
	const int mode = gParams.mode;

	float4 viewPos = input[0].position;
	float density = input[0].density;
	unsigned int phase = input[0].phase;
	float4 gl_Vertex = input[0].vertex;

	//float gl_PointSize = -pointScale * (pointRadius / viewPos.z);
	//float spriteSize = (pointRadius / viewPos.z);
	float spriteSize = pointRadius * 2;

	PointGeoOut output;

	for (int i = 0; i < 4; ++i)
	{

		float4 eyePos = viewPos;									// start with point position
		eyePos.xy += spriteSize * (corners[i] - float2(0.5, 0.5));	// add corner position
		gl_Position = mul(gParams.projection, eyePos);				// complete transformation

		gl_TexCoord[0].xy = corners[i].xy;							// use corner as texCoord
		gl_TexCoord[0].y = 1.0f - gl_TexCoord[0].y;					// flip the y component of uv (glsl to hlsl conversion)
		gl_TexCoord[1] = mul(lightTransform, float4(gl_Vertex.xyz - lightDir * pointRadius * 2.0, 1.0));
		gl_TexCoord[2] = mul(gl_ModelViewMatrix, float4(lightDir, 0.0));

		if (mode == 1)
		{
			// density visualization
			if (density < 0.0f)
				gl_TexCoord[3].xyz = lerp(float3(0.1, 0.1, 1.0), float3(0.1, 1.0, 1.0), -density);
			else
				gl_TexCoord[3].xyz = lerp(float3(1.0, 1.0, 1.0), float3(0.1, 0.2, 1.0), density);
		}
		else if (mode == 2)
		{
			//gl_PointSize *= clamp(gl_Vertex.w * 0.25, 0.0f, 1.0);
			float tmp = clamp(gl_Vertex.w * 0.05, 0.0f, 1.0);
			gl_TexCoord[3].xyzw = float4(tmp, tmp, tmp, tmp);
		}
		else
		{
			gl_TexCoord[3].xyz = lerp(gParams.colors[phase % 8].xyz * 2.0, float3(1.0, 1.0, 1.0), 0.1);
		}

		gl_TexCoord[4].xyz = gl_Vertex.xyz;
		gl_TexCoord[5].xyz = viewPos.xyz;

		output.position = gl_Position;
		[unroll]
		for (int j = 0; j < 6; j++)
			output.texCoord[j] = gl_TexCoord[j];

		triStream.Append(output);
	}
}
