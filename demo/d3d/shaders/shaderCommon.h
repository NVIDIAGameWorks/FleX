struct MeshShaderConst
{
	float4x4 modelViewProjection;
	float4x4 modelView;
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
	int increaseGfxLoadForAsyncComputeTesting;
};

struct DebugRenderConst
{
	float4x4 modelView;
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
	//float clipDistance[1] : CLIP_DISTANCE;

	float3 worldNormal: TEXCOORD0;						///< Normal in world space
	float4 lightOffsetPosition: TEXCOORD1;				///< Position in light space (offset slightly)
	float3 viewLightDir: TEXCOORD2;						///< Light direction in view space
	float3 worldPosition: TEXCOORD3;					///< Position in worldspace
	float4 color: TEXCOORD4;							///< Color
	float2 texCoord: TEXCOORD5;							///< Tex coords
	float4 secondaryColor: TEXCOORD6;					///< Secondary color
	float4 viewPosition: TEXCOORD7;						///< Position in view space
};
#endif

struct PointShaderConst
{
	float4x4 modelView;
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
	float4 viewPosition : POSITION;
	float density : DENSITY;
	int phase : PHASE;
	float4 modelPosition : VERTEX;
};

struct PointGeoOut
{
	float4 position : SV_POSITION;
	
	float2 texCoord: TEXCOORD0;
	float4 lightOffsetPosition: TEXCOORD1;
	float3 viewLightDir: TEXCOORD2;				//< Light direction in view space 
	float4 reflectance: TEXCOORD3;
	float3 modelPosition: TEXCOORD4;			///< Model space position
	float3 viewPosition : TEXCOORD5;			///< View space position
};

#endif

struct FluidShaderConst
{
	float4x4 modelViewProjection;
	float4x4 modelView;
	float4x4 projection;			// ogl projection
	float4x4 inverseModelView;
	float4x4 inverseProjection;		// ogl inverse projection

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
	//float _pad3;
	float pointRadius;  // point size in world space

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
	float4 bounds: TEXCOORD0;				// xmin, xmax, ymin, ymax
	float4 invQ0: TEXCOORD1;
	float4 invQ1: TEXCOORD2;
	float4 invQ2: TEXCOORD3;
	float4 invQ3: TEXCOORD4;
	float4 ndcPos: TEXCOORD5;				/// Position in normalized device coordinates (ie /w)
};

struct FluidGeoOut
{
	float4 position : SV_POSITION;
	float4 invQ0 : TEXCOORD0;
	float4 invQ1 : TEXCOORD1;
	float4 invQ2 : TEXCOORD2;
	float4 invQ3 : TEXCOORD3;
};

struct PassthroughVertexOut
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD;
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
	float4 velocity : VELOCITY;		// holding velocity in u
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

	float2 uv : UV;

};

#endif

