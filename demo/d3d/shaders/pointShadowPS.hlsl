#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	PointShaderConst gParams;
};

void pointShadowPS(PointGeoOut input) 
{
	// calculate normal from texture coordinates
	float2 normal = input.texCoord.xy - float2(0.5, 0.5);
	float mag = dot(normal.xy, normal.xy);
	if (mag > 0.5 * 0.5) 
	{
		discard;   // kill pixels outside circle
	}
}
