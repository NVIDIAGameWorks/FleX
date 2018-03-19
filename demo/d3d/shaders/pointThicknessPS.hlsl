#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	FluidShaderConst gParams;
};

float4 pointThicknessPS(FluidGeoOut input) : SV_TARGET
{
	float4 gl_FragColor;
	float4 gl_TexCoord[1];

	//[unroll]
	//for (int i = 0; i < 2; i++)
	//	gl_TexCoord[i] = input.texCoord[i];
	gl_TexCoord[0] = input.invQ0;

	// calculate normal from texture coordinates
	float3 normal;
	normal.xy = gl_TexCoord[0].xy * float2(2.0, -2.0) + float2(-1.0, 1.0);
	float mag = dot(normal.xy, normal.xy);
	if (mag > 1.0) discard;   // kill pixels outside circle
	normal.z = sqrt(1.0 - mag);

	float tmp = normal.z * 0.005;
	gl_FragColor = float4(tmp, tmp, tmp, tmp);

	return gl_FragColor;
}
