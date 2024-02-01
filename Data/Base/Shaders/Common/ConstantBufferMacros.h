#pragma once

#include "Platforms.h"

#if PL_ENABLED(PLATFORM_SHADER)

// HLSL

#  define PL_SHADER_STRUCT

struct Transform
{
  float4 r0;
  float4 r1;
  float4 r2;
};

float4x4 TransformToMatrix(Transform t)
{
  return float4x4(t.r0, t.r1, t.r2, float4(0, 0, 0, 1));
}

float4 TransformToPosition(Transform t)
{
  return float4(t.r0.w, t.r1.w, t.r2.w, 1);
}

float3x3 TransformToRotation(Transform t)
{
  return float3x3(t.r0.xyz, t.r1.xyz, t.r2.xyz);
}

#  define CONSTANT_BUFFER(Name, Slot) cbuffer Name : register(b##Slot)
#  define CONSTANT_BUFFER2(Name, Slot, Set) cbuffer Name : register(b##Slot, space##Set)
#  define STRUCTURED_BUFFER(Name, Type) StructuredBuffer<Type> Name
#  define FLOAT1(Name) float Name
#  define FLOAT2(Name) float2 Name
#  define FLOAT3(Name) float3 Name
#  define FLOAT4(Name) float4 Name
#  define INT1(Name) int Name
#  define INT2(Name) int2 Name
#  define INT3(Name) int3 Name
#  define INT4(Name) int4 Name
#  define UINT1(Name) uint Name
#  define UINT2(Name) uint2 Name
#  define UINT3(Name) uint3 Name
#  define UINT4(Name) uint4 Name
#  define MAT3(Name) float3x3 Name
#  define MAT4(Name) float4x4 Name
#  define TRANSFORM(Name) Transform Name
#  define COLOR4F(Name) float4 Name
#  define COLOR4UB(Name) uint Name
#  define BOOL1(Name) bool Name
#  define PACKEDHALF2(Name1, Name2, CombinedName) uint CombinedName
#  define PACKEDCOLOR4H(Name) \
    uint PL_CONCAT(Name, RG); \
    uint PL_CONCAT(Name, GB)

#  define UNPACKHALF2(Name1, Name2, CombinedName) \
    float Name1 = f16tof32(CombinedName);         \
    float Name2 = f16tof32(CombinedName >> 16)
#  define UNPACKCOLOR4H(Name) RGBA16FToFloat4(PL_CONCAT(Name, RG), PL_CONCAT(Name, GB))

#else

// C++

#  include <Foundation/Basics/Platform/Common.h>
#  include <RendererFoundation/Shader/Types.h>

#  define PL_SHADER_STRUCT alignas(16)
#  define CONSTANT_BUFFER(Name, Slot) struct alignas(16) Name
#  define CONSTANT_BUFFER2(Name, Slot, Set) struct alignas(16) Name
#  define STRUCTURED_BUFFER(Name, Type)
#  define BEGIN_PUSH_CONSTANTS(Name) struct PL_SHADER_STRUCT Name
#  define END_PUSH_CONSTANTS(Name) ;
#  define FLOAT1(Name) float Name
#  define FLOAT2(Name) plVec2 Name
#  define FLOAT3(Name) plVec3 Name
#  define FLOAT4(Name) plVec4 Name
#  define INT1(Name) int Name
#  define INT2(Name) plVec2I32 Name
#  define INT3(Name) plVec3I32 Name
#  define INT4(Name) plVec4I32 Name
#  define UINT1(Name) plUInt32 Name
#  define UINT2(Name) plVec2U32 Name
#  define UINT3(Name) plVec3U32 Name
#  define UINT4(Name) plVec4U32 Name
#  define MAT3(Name) plShaderMat3 Name
#  define MAT4(Name) plMat4 Name
#  define TRANSFORM(Name) plShaderTransform Name
#  define COLOR4F(Name) plColor Name
#  define COLOR4UB(Name) plColorGammaUB Name
#  define BOOL1(Name) plShaderBool Name
#  define PACKEDHALF2(Name1, Name2, CombinedName) \
    plFloat16 Name1;                              \
    plFloat16 Name2
#  define PACKEDCOLOR4H(Name) plColorLinear16f Name

#endif
