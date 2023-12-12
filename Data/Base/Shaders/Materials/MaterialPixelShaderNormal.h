#pragma once

#if BLEND_MODE == BLEND_MODE_MASKED && RENDER_PASS != RENDER_PASS_WIREFRAME

// No need to do alpha test again if we have a depth prepass
#if defined(FORWARD_PASS_WRITE_DEPTH) && (RENDER_PASS == RENDER_PASS_FORWARD || RENDER_PASS == RENDER_PASS_DEFERRED || RENDER_PASS == RENDER_PASS_EDITOR)
#if FORWARD_PASS_WRITE_DEPTH == TRUE
#define USE_ALPHA_TEST
#endif
#else
#define USE_ALPHA_TEST
#endif

#if defined(USE_ALPHA_TEST) && defined(MSAA)
#if MSAA == TRUE
#define USE_ALPHA_TEST_SUPER_SAMPLING
#endif
#endif

#endif

#include <Shaders/Common/Lighting.h>
#include <Shaders/Materials/MaterialHelper.h>

struct PS_OUT
{
#if RENDER_PASS != RENDER_PASS_DEPTH_ONLY
  float4 Color    : SV_Target0;
  float4 Velocity : SV_Target1;


#if RENDER_PASS == RENDER_PASS_DEFERRED
  float4 Normal   : SV_Target2;
  float4 Material : SV_Target3;
  float4 MaterialAdvanced : SV_Target4;
  float4 ClearCoat : SV_Target5;
  float4 Subsurface : SV_Target6;
#endif
#endif

#if defined(USE_ALPHA_TEST_SUPER_SAMPLING)
  uint Coverage   : SV_Coverage;
#endif
};

PS_OUT main(PS_IN Input)
{
#if CAMERA_MODE == CAMERA_MODE_STEREO
  s_ActiveCameraEyeIndex = Input.RenderTargetArrayIndex;
#endif

  G.Input = Input;
#if defined(CUSTOM_GLOBALS)
  FillCustomGlobals();
#endif

  PS_OUT Output;

#if defined(USE_ALPHA_TEST)
  uint coverage = CalculateCoverage();
  if (coverage == 0)
  {
    discard;
  }

#if defined(USE_ALPHA_TEST_SUPER_SAMPLING)
  Output.Coverage = coverage;
#endif
#endif

  plMaterialData matData = FillMaterialData();

  uint gameObjectId = GetInstanceData().GameObjectID;

#if RENDER_PASS == RENDER_PASS_FORWARD || RENDER_PASS == RENDER_PASS_EDITOR
  plPerClusterData clusterData = GetClusterData(Input.Position.xyw);

#if defined(USE_DECALS)
  ApplyDecals(matData, clusterData, gameObjectId);
#endif

#if RENDER_PASS == RENDER_PASS_EDITOR
  if (RenderPass == EDITOR_RENDER_PASS_DIFFUSE_LIT_ONLY)
  {
    matData.diffuseColor = 0.5;
  }
#endif

#if SHADING_MODE == SHADING_MODE_LIT
#if BLEND_MODE == BLEND_MODE_OPAQUE || BLEND_MODE == BLEND_MODE_MASKED
  bool applySSAO = true;
#else
  bool applySSAO = false;
#endif

  AccumulatedLight light = CalculateLighting(matData, clusterData, Input.Position.xyw, applySSAO);
#else
  AccumulatedLight light = InitializeLight(matData.diffuseColor, 0.0f);
#endif

#if BLEND_MODE != BLEND_MODE_OPAQUE && BLEND_MODE != BLEND_MODE_MASKED
#if defined(USE_MATERIAL_REFRACTION)
  ApplyRefraction(matData, light);
#endif

  float specularNormalization = lerp(1.0f, 1.0f / matData.opacity, saturate(matData.opacity * 10.0f));
  light.specularLight *= specularNormalization;
#endif

  float3 litColor = light.diffuseLight + light.specularLight;
  litColor *= Exposure;
  litColor += matData.emissiveColor;
  litColor *= matData.cavity;

#endif

#if RENDER_PASS == RENDER_PASS_FORWARD
#if defined(USE_FOG)
#if BLEND_MODE == BLEND_MODE_ADDITIVE
  matData.opacity *= GetFogAmount(matData.worldPosition);
#elif BLEND_MODE == BLEND_MODE_MODULATE
  litColor = lerp(1.0, litColor, GetFogAmount(matData.worldPosition));
#else
  litColor = ApplyFog(litColor, matData.worldPosition, GetFogAmount(matData.worldPosition));
#endif
#endif

  Output.Color = float4(litColor, matData.opacity);
#if defined(USE_VELOCITY)
  Output.Velocity = float4(matData.velocity.x, matData.velocity.y, 0.0f, 0.0f);
#endif

#elif RENDER_PASS == RENDER_PASS_EDITOR
  if (RenderPass == EDITOR_RENDER_PASS_DIFFUSE_LIT_ONLY)
  {
    Output.Color = float4(SrgbToLinear(light.diffuseLight ), matData.opacity);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_SPECULAR_LIT_ONLY)
  {
    Output.Color = float4(SrgbToLinear(light.specularLight ), matData.opacity);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_LIGHT_COUNT || RenderPass == EDITOR_RENDER_PASS_DECAL_COUNT)
  {
    float lightCount = RenderPass == EDITOR_RENDER_PASS_LIGHT_COUNT ? GET_LIGHT_INDEX(clusterData.counts) : GET_DECAL_INDEX(clusterData.counts);
    float3 heatmap = 0;
    if (lightCount > 0)
    {
      float x = (lightCount - 1) / 16;
      heatmap.r = saturate(x);
      heatmap.g = saturate(2 - x);
    }

    Output.Color = float4(lerp(litColor, heatmap, 0.7), 1);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_TEXCOORDS_UV0)
  {
#if defined(USE_TEXCOORD0)
    Output.Color = float4(SrgbToLinear(float3(frac(Input.TexCoord0.xy), 0)), 1);
#else
    Output.Color = float4(0, 0, 0, 1);
#endif
  }
  else if (RenderPass == EDITOR_RENDER_PASS_TEXCOORDS_UV1)
  {
#if defined(USE_TEXCOORD1)
    Output.Color = float4(SrgbToLinear(float3(frac(Input.TexCoord1.xy), 0)), 1);
#else
    Output.Color = float4(0, 0, 0, 1);
#endif
  }
  else if (RenderPass == EDITOR_RENDER_PASS_VERTEX_COLORS0)
  {
#if defined(USE_COLOR0)
    Output.Color = float4(SrgbToLinear(Input.Color0.rgb), 1);
#else
    Output.Color = float4(0, 0, 0, 1);
#endif
  }
  else if (RenderPass == EDITOR_RENDER_PASS_VERTEX_COLORS1)
  {
#if defined(USE_COLOR1)
    Output.Color = float4(SrgbToLinear(Input.Color1.rgb), 1);
#else
    Output.Color = float4(0, 0, 0, 1);
#endif
  }
  else if (RenderPass == EDITOR_RENDER_PASS_VERTEX_NORMALS)
  {
#if defined(USE_NORMAL)
    Output.Color = float4(SrgbToLinear(normalize(Input.Normal) * 0.5 + 0.5), 1);
#else
    Output.Color = float4(0, 0, 0, 1);
#endif
  }
  else if (RenderPass == EDITOR_RENDER_PASS_VERTEX_TANGENTS)
  {
#if defined(USE_TANGENT)
    Output.Color = float4(SrgbToLinear(normalize(Input.Tangent) * 0.5 + 0.5), 1);
#else
    Output.Color = float4(0, 0, 0, 1);
#endif
  }
  else if (RenderPass == EDITOR_RENDER_PASS_PIXEL_NORMALS)
  {
    Output.Color = float4(SrgbToLinear(matData.worldNormal * 0.5 + 0.5), 1);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_DIFFUSE_COLOR)
  {
    Output.Color = float4(matData.diffuseColor, matData.opacity);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE)
  {
    Output.Color = float4(matData.diffuseColor, matData.opacity);

    float luminance = GetLuminance(matData.diffuseColor);
    if (luminance < 0.017) // 40 srgb
    {
      Output.Color = float4(1, 0, 1, matData.opacity);
    }
    else if (luminance > 0.9) // 243 srgb
    {
      Output.Color = float4(0, 1, 0, matData.opacity);
    }
  }
  else if (RenderPass == EDITOR_RENDER_PASS_SPECULAR_COLOR)
  {
    Output.Color = float4(matData.specularColor, matData.opacity);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_EMISSIVE_COLOR)
  {
    Output.Color = float4(matData.emissiveColor, matData.opacity);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_ROUGHNESS)
  {
    Output.Color = float4(SrgbToLinear(matData.roughness), matData.opacity);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_OCCLUSION)
  {
    float ssao = SampleSSAO(Input.Position.xyw);
    float occlusion = matData.occlusion * ssao;

    Output.Color = float4(SrgbToLinear(occlusion), matData.opacity);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_MOTIONVECTORS)
  {
#if defined(USE_VELOCITY)
    float3 outputVelocity = (float3(matData.velocity, 0.0f));
    Output.Color = float4(outputVelocity, matData.opacity);
#else
    Output.Color = float4(0.0, 0.0, 0.0, matData.opacity);
#endif
  }
  else if (RenderPass == EDITOR_RENDER_PASS_DEPTH)
  {
    float depth = Input.Position.w * ClipPlanes.z;
    Output.Color = float4(SrgbToLinear(depth), 1);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_STATIC_VS_DYNAMIC)
  {
    Output.Color = ColorizeGameObjectId(gameObjectId);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_BONE_WEIGHTS)
  {
#if defined(USE_DEBUG_INTERPOLATOR)
    Output.Color = Input.DebugInterpolator;
#else
    Output.Color = float4(0.05, 0.05, 0.1, 1);
#endif
  }
  else
  {
    Output.Color = float4(1.0f, 0.0f, 1.0f, 1.0f);
  }

#elif RENDER_PASS == RENDER_PASS_WIREFRAME
  if (RenderPass == WIREFRAME_RENDER_PASS_MONOCHROME)
  {
    Output.Color = float4(0.4f, 0.4f, 0.4f, 1.0f);
  }
  else
  {
    Output.Color = float4(matData.diffuseColor, 1.0f);
  }

#elif (RENDER_PASS == RENDER_PASS_PICKING || RENDER_PASS == RENDER_PASS_PICKING_WIREFRAME)
  Output.Color = RGBA8ToFloat4(gameObjectId);

#elif RENDER_PASS == RENDER_PASS_DEPTH_ONLY

#elif RENDER_PASS == RENDER_PASS_DEFERRED
  Output.Color = float4(matData.diffuseColor + lerp(0.0f, matData.specularColor, matData.metalness) + matData.emissiveColor, GetLuminance(matData.emissiveColor));
  Output.Velocity = float4(matData.velocity.x, matData.velocity.y, pack_floats(matData.specularColor.x, matData.specularColor.y), matData.specularColor.z);
  Output.Normal   = float4(matData.worldNormal, pack_uint32_to_float16(gameObjectId));
  Output.Material = float4(pack_floats(matData.roughness, matData.metalness), pack_floats(matData.subsurfaceStrength, matData.subsurfaceScatterPower), pack_floats(matData.occlusion, matData.cavity), pack_floats(matData.sheen, matData.sheenTintFactor));
  Output.MaterialAdvanced = float4(matData.subsurfaceColor, pack_floats(matData.anisotropic, matData.anisotropicRotation));
  Output.ClearCoat = float4(matData.clearcoatNormal, pack_floats(matData.clearcoat, matData.clearcoatRoughness));

#else
  Output.Color = float4(litColor, matData.opacity);
#error "RENDER_PASS uses undefined value."
#endif

  return Output;
}
