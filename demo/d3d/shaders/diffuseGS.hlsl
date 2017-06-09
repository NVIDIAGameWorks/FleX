#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	DiffuseShaderConst gParams;
};

static const float2 corners[4] =
{
	float2(0.0, 1.0),
	float2(0.0, 0.0),
	float2(1.0, 1.0),
	float2(1.0, 0.0)
};

[maxvertexcount(4)]
void diffuseGS(point DiffuseVertexOut input[1], inout TriangleStream<DiffuseGeometryOut> triStream)
{
	float4 ndcPos = input[0].ndcPos;

	// frustrum culling
	const float ndcBound = 1.0;
	if (ndcPos.x < -ndcBound) return;
	if (ndcPos.x > ndcBound) return;
	if (ndcPos.y < -ndcBound) return;
	if (ndcPos.y > ndcBound) return;

	float pointScale = gParams.diffuseScale;
	float velocityScale = 1.0;

	float3 v = input[0].viewVel.xyz;
	float3 p = input[0].viewPos.xyz;
		
	// billboard in eye space
	float3 u = float3(0.0, pointScale, 0.0);
	float3 l = float3(pointScale, 0.0, 0.0);
	
	// increase size based on life
	float lifeTime = input[0].worldPos.w;

	float lifeFade = lerp(1.0f + gParams.diffusion, 1.0, min(1.0, lifeTime*0.25f));
	u *= lifeFade;
	l *= lifeFade;

	float fade = 1.0/(lifeFade*lifeFade);
	float vlen = length(v)*gParams.motionBlurScale;
	
	if (vlen > 0.5)
	{
		float len = max(pointScale, vlen*0.016);
		fade = min(1.0, 2.0/(len/pointScale));

		u = normalize(v)*max(pointScale, vlen*0.016);	// assume 60hz
		l = normalize(cross(u, float3(0.0, 0.0, -1.0)))*pointScale;
	}
	
	{
		DiffuseGeometryOut output;
		
		output.worldPos = input[0].worldPos;	// vertex world pos (life in w)
		output.viewPos = input[0].viewPos;		// vertex eye pos
		output.viewVel.xyz = input[0].viewVel.xyz; // vertex velocity in view space
		output.viewVel.w = fade;
		output.lightDir = mul(gParams.modelView, float4(gParams.lightDir, 0.0));
		output.color = input[0].color;

		output.uv = float2(0.0, 1.0);      
		output.clipPos = mul(gParams.projection, float4(p + u - l, 1.0));
		triStream.Append(output);
		
		output.uv = float2(0.0, 0.0);
        output.clipPos = mul(gParams.projection, float4(p - u - l, 1.0));
        triStream.Append(output);

		output.uv = float2(1.0, 1.0);
        output.clipPos = mul(gParams.projection, float4(p + u + l, 1.0));
        triStream.Append(output);

		output.uv = float2(1.0, 0.0);
        output.clipPos = mul(gParams.projection, float4(p - u + l, 1.0));
        triStream.Append(output);
    }
}
