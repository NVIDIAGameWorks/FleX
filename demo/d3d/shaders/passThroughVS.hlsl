#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	FluidShaderConst gParams;
};

PassthroughVertexOut passThroughVS(MeshVertexIn input)
{
	PassthroughVertexOut output;
	output.position = float4(input.position, 1.0f);
	output.texCoord = input.texCoord;
	return output;
}
