#include "shaderCommon.h"

cbuffer constBuf : register(b0)
{
	FluidShaderConst gParams;
};

Texture2D<float> depthTex : register(t0);

float sqr(float x) { return x*x; }

float4 blurDepthPS(PassthroughVertexOut input) : SV_TARGET
{
	float4 gl_FragColor = float4(0.0, 0.0, 0.0, 0.0);
	float4 gl_FragCoord = input.position;

	// debug: return the center depth sample
	//float d = depthTex.Load(int3(gl_FragCoord.xy, 0)).x;
	//return d;

	const float blurRadiusWorld = gParams.blurRadiusWorld;
	const float blurScale = gParams.blurScale;
	const float blurFalloff = gParams.blurFalloff;

	// eye-space depth of center sample
	float depth = depthTex.Load(int3(gl_FragCoord.xy, 0)).x;
	float thickness = 0.0f; //texture2D(thicknessTex, gl_TexCoord[0].xy).x;

	/*
	// threshold on thickness to create nice smooth silhouettes
	if (depth == 0.0)
	{
		gl_FragColor.x = 0.0;
		return gl_FragColor;
	}
	*/

	float blurDepthFalloff = 5.5;
	float maxBlurRadius = 5.0;

	//discontinuities between different tap counts are visible. to avoid this we 
	//use fractional contributions between #taps = ceil(radius) and floor(radius) 
	float radius = min(maxBlurRadius, blurScale * (blurRadiusWorld / -depth));
	float radiusInv = 1.0 / radius;
	float taps = ceil(radius);
	float frac = taps - radius;

	float sum = 0.0;
	float wsum = 0.0;
	float count = 0.0;

	for (float y = -taps; y <= taps; y += 1.0)
	{
		for (float x = -taps; x <= taps; x += 1.0)
		{
			float2 offset = float2(x, y);

			//float sample = texture2DRect(depthTex, gl_FragCoord.xy + offset).x;
			float sample = depthTex.Load(int3(gl_FragCoord.xy + offset, 0)).x;

			//if (sample < -10000.0 * 0.5)
				//continue;

			// spatial domain
			float r1 = length(float2(x, y))*radiusInv;
			float w = exp(-(r1*r1));

			// range domain (based on depth difference)
			float r2 = (sample - depth) * blurDepthFalloff;
			float g = exp(-(r2*r2));

			//fractional radius contributions
			float wBoundary = step(radius, max(abs(x), abs(y)));
			float wFrac = 1.0 - wBoundary*frac;

			sum += sample * w * g * wFrac;
			wsum += w * g * wFrac;
			count += g * wFrac;
		}
	}

	if (wsum > 0.0)
	{
		sum /= wsum;
	}

	float blend = count / sqr(2.0 * radius + 1.0);
	gl_FragColor.x = lerp(depth, sum, blend);

	return gl_FragColor;
}
