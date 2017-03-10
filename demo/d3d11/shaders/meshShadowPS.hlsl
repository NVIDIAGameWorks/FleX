#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	MeshShaderConst gParams;
};

float4 meshPS_Shadow(MeshVertexOut input) : SV_TARGET
{
	return float4(0.0, 0.0, 0.0, 1.0);
}
