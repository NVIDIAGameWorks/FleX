#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	FluidShaderConst gParams;
};

PassthroughVertexOut passThroughVS(PassthroughVertexIn input)
{
	float4 gl_Vertex = float4(input.position, 0.0f, 1.0f);
	float2 gl_MultiTexCoord0 = input.texCoord;

	PassthroughVertexOut output;
	output.position = gl_Vertex;
	output.texCoord[0] = gl_MultiTexCoord0;

	return output;

	/*
	void main()
	{
		gl_Position = vec4(gl_Vertex.xyz, 1.0);
		gl_TexCoord[0] = gl_MultiTexCoord0;
	}
	*/
}
