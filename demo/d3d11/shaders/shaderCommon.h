struct MeshShaderConst
{
	float4x4 modelviewprojection;
	float4x4 modelview;
	float4x4 objectTransform;
	float4x4 lightTransform;

	float4 clipPlane;
	float4 fogColor;
	float4 color;
	float4 secondaryColor;

	float4 shadowTaps[12];

	float3 lightPos;
	float _pad0;
	float3 lightDir;
	float _pad1;

	float bias;
	float expand;
	float spotMin;
	float spotMax;

	int grid;
	int tex;
	int colorArray;
	int _pad2;
};

struct DebugRenderConst
{
	float4x4 modelview;
	float4x4 projection;
};


#ifndef EXCLUDE_SHADER_STRUCTS
struct MeshVertexIn
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 texCoord : TEXCOORD;
	float4 color : COLOR;
};

struct MeshVertexOut
{
	float4 position : SV_POSITION;
	//float3 normal : NORMAL;
	//float4 color : COLOR;
	float4 texCoord[8] : TEXCOORD;
	//float clipDistance[1] : CLIP_DISTANCE;
};
#endif

struct PointShaderConst
{
	float4x4 modelview;
	float4x4 projection;
	float4x4 lightTransform;

	float4 colors[8];
	float4 shadowTaps[12];

	float3 lightPos;
	float _pad0;
	float3 lightDir;
	float _pad1;

	float pointRadius;  // point size in world space
	float pointScale;   // scale to calculate size in pixels
	float spotMin;
	float spotMax;

	int mode;
	int _pad2[3];
};

#ifndef EXCLUDE_SHADER_STRUCTS
struct PointVertexIn
{
	float4 position : POSITION;
	float density : DENSITY;
	int phase : PHASE;
};

struct PointVertexOut
{
	float4 position : POSITION;
	float density : DENSITY;
	int phase : PHASE;
	float4 vertex : VERTEX;
};

struct PointGeoOut
{
	float4 position : SV_POSITION;
	float4 texCoord[6] : TEXCOORD;
};
#endif

struct FluidShaderConst
{
	float4x4 modelviewprojection;
	float4x4 modelview;
	float4x4 projection;			// ogl projection
	float4x4 modelview_inverse;
	float4x4 projection_inverse;	// ogl inverse projection

	float4 invTexScale;

	float3 invViewport;
	float _pad0;
	//float3 invProjection;
	//float _pad1;

	float blurRadiusWorld;
	float blurScale;
	float blurFalloff;
	int debug;

	float3 lightPos;
	float _pad1;
	float3 lightDir;
	float _pad2;
	float4x4 lightTransform;

	float4 color;
	float4 clipPosToEye;

	float spotMin;
	float spotMax;
	float ior;
	float _pad3;

	float4 shadowTaps[12];
};

#ifndef EXCLUDE_SHADER_STRUCTS
struct FluidVertexIn
{
	float4 position : POSITION;
	float4 q1 : U;
	float4 q2 : V;
	float4 q3 : W;
};

struct FluidVertexOut
{
	float4 position : POSITION;
	float4 texCoord[6] : TEXCOORD;
};

struct FluidGeoOut
{
	float4 position : SV_POSITION;
	float4 texCoord[4] : TEXCOORD;
};

struct PassthroughVertexIn
{
	float2 position : POSITION;
	float2 texCoord : TEXCOORD;
};

struct PassthroughVertexOut
{
	float4 position : SV_POSITION;
	float2 texCoord[1] : TEXCOORD;
};
#endif


struct DiffuseShaderConst
{
	float3 lightPos; float pad0;
	float3 lightDir; float pad1;
	float4x4 lightTransform;
	float4 color;

	float4x4 modelView;
	float4x4 modelViewProjection;
	float4x4 projection;

	float4 shadowTaps[12];

	float diffusion;
	float diffuseRadius;  // point size in world space
	float diffuseScale;   // scale to calculate size in pixels
	
	float spotMin;
	float spotMax;

	float motionBlurScale;

	float pad3;
	float pad4;

};

#ifndef EXCLUDE_SHADER_STRUCTS

struct DiffuseVertexIn
{
	float4 position : POSITION; // lifetime in w
	float4 velocity : VELOCITY;
};

struct DiffuseVertexOut
{
	float4 worldPos : POSITION;	// lifetime in w
	float4 ndcPos : NCDPOS;
	float4 viewPos : VIEWPOS;
	float4 viewVel : VIEWVEL;		
	
	float4 color : COLOR;
};

struct DiffuseGeometryOut
{
	float4 clipPos : SV_POSITION;

	float4 worldPos : POSITION;

	float4 viewPos : VIEWPOS;
	float4 viewVel : VIEWVEL;		
	
	float4 lightDir : LIGHTDIR;
	float4 color : COLOR;

	float4 uv : UV;

};

#endif

