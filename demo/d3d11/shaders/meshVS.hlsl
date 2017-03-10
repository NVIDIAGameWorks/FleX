#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	MeshShaderConst gParams;
};

MeshVertexOut meshVS(MeshVertexIn input)
{
	float4 gl_Position;
	float4 gl_TexCoord[8];

	{
		[unroll]
		for (int i = 0; i < 8; i++)
			gl_TexCoord[i] = float4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	const float4x4 gl_ModelViewProjectionMatrix = gParams.modelviewprojection;
	const float4x4 gl_ModelViewMatrix = gParams.modelview;
	const float4x4 objectTransform  = gParams.objectTransform;
	const float4x4 lightTransform = gParams.lightTransform;
	const float3 lightDir = gParams.lightDir;
	const float bias = gParams.bias;
	const float4 clipPlane = gParams.clipPlane;
	const float expand = gParams.expand;
	const float4 gl_Color = gParams.color;
	const float4 gl_SecondaryColor = gParams.secondaryColor;

	const float3 gl_Vertex = input.position;
	const float3 gl_Normal = input.normal;
	const float2 gl_MultiTexCoord0 = input.texCoord;

	float3 n = normalize(mul(objectTransform, float4(gl_Normal, 0.0)).xyz);
	float3 p = mul(objectTransform, float4(gl_Vertex.xyz, 1.0)).xyz;

	// calculate window-space point size
	gl_Position = mul(gl_ModelViewProjectionMatrix, float4(p + expand * n, 1.0));

	gl_TexCoord[0].xyz = n;
	gl_TexCoord[1] = mul(lightTransform, float4(p + n * bias, 1.0));
	gl_TexCoord[2] = mul(gl_ModelViewMatrix, float4(lightDir, 0.0));
	gl_TexCoord[3].xyz = p;
	if (gParams.colorArray)
		gl_TexCoord[4] = input.color;
	else
		gl_TexCoord[4] = gl_Color;
	gl_TexCoord[5].xy = gl_MultiTexCoord0;
	gl_TexCoord[5].y = 1.0f - gl_TexCoord[5].y;					// flip the y component of uv (glsl to hlsl conversion)
	gl_TexCoord[6] = gl_SecondaryColor;
	gl_TexCoord[7] = mul(gl_ModelViewMatrix, float4(gl_Vertex.xyz, 1.0));

	MeshVertexOut output;
	output.position = gl_Position;
	{
		[unroll]
		for (int i = 0; i < 8; i++)
			output.texCoord[i] = gl_TexCoord[i];
	}

	return output;

	/*
	uniform mat4 lightTransform;
	uniform vec3 lightDir;
	uniform float bias;
	uniform vec4 clipPlane;
	uniform float expand;

	uniform mat4 objectTransform;

	void main()
	{
		vec3 n = normalize((objectTransform*vec4(gl_Normal, 0.0)).xyz);
		vec3 p = (objectTransform*vec4(gl_Vertex.xyz, 1.0)).xyz;

		// calculate window-space point size
		gl_Position = gl_ModelViewProjectionMatrix * vec4(p + expand*n, 1.0);

		gl_TexCoord[0].xyz = n;
		gl_TexCoord[1] = lightTransform*vec4(p + n*bias, 1.0);
		gl_TexCoord[2] = gl_ModelViewMatrix*vec4(lightDir, 0.0);
		gl_TexCoord[3].xyz = p;
		gl_TexCoord[4] = gl_Color;
		gl_TexCoord[5] = gl_MultiTexCoord0;
		gl_TexCoord[6] = gl_SecondaryColor;
		gl_TexCoord[7] = gl_ModelViewMatrix*vec4(gl_Vertex.xyz, 1.0);

		gl_ClipDistance[0] = dot(clipPlane, vec4(gl_Vertex.xyz, 1.0));
	*/
}
