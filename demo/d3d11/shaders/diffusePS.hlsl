#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	DiffuseShaderConst gParams;
};

float sqr(float x) { return x * x; }


float4 diffusePS(DiffuseGeometryOut input
	//, out float gl_FragDepth : SV_DEPTH
) : SV_TARGET
{
	//return float4(1.0f, 0.0f, 0.0f, 1.0f);

	float attenuation = 1.0f;
	float lifeTime = input.worldPos.w;
	float lifeFade = min(1.0, lifeTime*0.125);
	float velocityFade = input.viewVel.w;

    // calculate normal from texture coordinates
    float3 normal;
    normal.xy = input.uv.xy*float2(2.0, 2.0) + float2(-1.0, -1.0);
    float mag = dot(normal.xy, normal.xy);
    
	// kill pixels outside circle
	if (mag > 1.0)
		discard;   

   	normal.z = 1.0-mag;

	float alpha = lifeFade*velocityFade*sqr(normal.z);

	return float4(alpha, alpha, alpha, alpha);

}
