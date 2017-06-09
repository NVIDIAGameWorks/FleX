#ifndef SHADER_COMMON_D3D_H
#define SHADER_COMMON_D3D_H

#include <DirectXMath.h>

struct ShadowMap;

namespace Hlsl { 

typedef DirectX::XMFLOAT3 float3;
typedef DirectX::XMFLOAT4 float4;
typedef DirectX::XMFLOAT4X4 float4x4;

#define EXCLUDE_SHADER_STRUCTS 1
#include "../d3d/shaders/shaderCommon.h"

} // namespace Shader

#endif // SHADER_COMMON_D3D_H
