#include <RendererFoundation/Resources/ResourceFormats.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>


namespace
{
  bool IsArrayViewInternal(const plGALTextureCreationDescription& texDesc, const plGALResourceViewCreationDescription& viewDesc)
  {
    return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
  }
  bool IsArrayViewInternal(const plGALTextureCreationDescription& texDesc, const plGALUnorderedAccessViewCreationDescription& viewDesc)
  {
    return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
  }
} // namespace

PL_ALWAYS_INLINE vk::SampleCountFlagBits plConversionUtilsVulkan::GetSamples(plEnum<plGALMSAASampleCount> samples)
{
  switch (samples)
  {
    case plGALMSAASampleCount::None:
      return vk::SampleCountFlagBits::e1;
    case plGALMSAASampleCount::TwoSamples:
      return vk::SampleCountFlagBits::e2;
    case plGALMSAASampleCount::FourSamples:
      return vk::SampleCountFlagBits::e4;
    case plGALMSAASampleCount::EightSamples:
      return vk::SampleCountFlagBits::e8;
    default:
      PL_ASSERT_NOT_IMPLEMENTED;
      return vk::SampleCountFlagBits::e1;
  }
}

PL_ALWAYS_INLINE vk::PresentModeKHR plConversionUtilsVulkan::GetPresentMode(plEnum<plGALPresentMode> presentMode, const plDynamicArray<vk::PresentModeKHR>& supportedModes)
{
  switch (presentMode)
  {
    case plGALPresentMode::Immediate:
    {
      if (supportedModes.Contains(vk::PresentModeKHR::eImmediate))
        return vk::PresentModeKHR::eImmediate;
      else if (supportedModes.Contains(vk::PresentModeKHR::eMailbox))
        return vk::PresentModeKHR::eMailbox;
      else
        return vk::PresentModeKHR::eFifo;
    }
    case plGALPresentMode::VSync:
      return vk::PresentModeKHR::eFifo; // FIFO must be supported according to the standard.
    default:
      PL_ASSERT_NOT_IMPLEMENTED;
      return vk::PresentModeKHR::eFifo;
  }
}

PL_ALWAYS_INLINE vk::ImageSubresourceRange plConversionUtilsVulkan::GetSubresourceRange(const plGALTextureCreationDescription& texDesc, const plGALRenderTargetViewCreationDescription& viewDesc)
{
  vk::ImageSubresourceRange range;
  plGALResourceFormat::Enum viewFormat = viewDesc.m_OverrideViewFormat == plGALResourceFormat::Invalid ? texDesc.m_Format : viewDesc.m_OverrideViewFormat;
  range.aspectMask = plGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  range.setBaseMipLevel(viewDesc.m_uiMipLevel).setLevelCount(1).setBaseArrayLayer(viewDesc.m_uiFirstSlice).setLayerCount(viewDesc.m_uiSliceCount);
  return range;
}

PL_ALWAYS_INLINE vk::ImageSubresourceRange plConversionUtilsVulkan::GetSubresourceRange(const plGALTextureCreationDescription& texDesc, const plGALResourceViewCreationDescription& viewDesc)
{
  vk::ImageSubresourceRange range;

  const bool bIsArrayView = IsArrayViewInternal(texDesc, viewDesc);

  plGALResourceFormat::Enum viewFormat = viewDesc.m_OverrideViewFormat == plGALResourceFormat::Invalid ? texDesc.m_Format : viewDesc.m_OverrideViewFormat;
  range.aspectMask = plGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (viewFormat == plGALResourceFormat::D24S8)
  {
    range.aspectMask |= vk::ImageAspectFlagBits::eStencil;
  }
  range.baseMipLevel = viewDesc.m_uiMostDetailedMipLevel;
  range.levelCount = plMath::Min(viewDesc.m_uiMipLevelsToUse, texDesc.m_uiMipLevelCount - range.baseMipLevel);

  switch (texDesc.m_Type)
  {
    case plGALTextureType::Texture2D:
    case plGALTextureType::Texture2DProxy:
    case plGALTextureType::Texture2DShared:
      range.layerCount = viewDesc.m_uiArraySize;
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      break;
    case plGALTextureType::TextureCube:
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      range.layerCount = viewDesc.m_uiArraySize * 6;
      break;
    case plGALTextureType::Texture3D:
      range.layerCount = 1;
      break;
    default:
      PL_ASSERT_NOT_IMPLEMENTED;
  }
  return range;
}


PL_ALWAYS_INLINE vk::ImageSubresourceRange plConversionUtilsVulkan::GetSubresourceRange(const plGALTextureCreationDescription& texDesc, const plGALUnorderedAccessViewCreationDescription& viewDesc)
{
  vk::ImageSubresourceRange range;

  const bool bIsArrayView = IsArrayViewInternal(texDesc, viewDesc);

  plGALResourceFormat::Enum viewFormat = viewDesc.m_OverrideViewFormat == plGALResourceFormat::Invalid ? texDesc.m_Format : viewDesc.m_OverrideViewFormat;
  range.aspectMask = plGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (viewFormat == plGALResourceFormat::D24S8)
  {
    range.aspectMask |= vk::ImageAspectFlagBits::eStencil;
  }

  range.baseMipLevel = viewDesc.m_uiMipLevelToUse;
  range.levelCount = 1;
  range.layerCount = viewDesc.m_uiArraySize;

  switch (texDesc.m_Type)
  {
    case plGALTextureType::Texture2D:
    case plGALTextureType::Texture2DProxy:
    case plGALTextureType::Texture2DShared:
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      break;
    case plGALTextureType::TextureCube:
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      break;
    case plGALTextureType::Texture3D:
      if (bIsArrayView)
      {
        PL_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      }
      break;
    default:
      PL_ASSERT_NOT_IMPLEMENTED;
  }
  return range;
}

PL_ALWAYS_INLINE vk::ImageSubresourceRange plConversionUtilsVulkan::GetSubresourceRange(
  const vk::ImageSubresourceLayers& layers)
{
  vk::ImageSubresourceRange range;
  range.aspectMask = layers.aspectMask;
  range.baseMipLevel = layers.mipLevel;
  range.levelCount = 1;
  range.baseArrayLayer = layers.baseArrayLayer;
  range.layerCount = layers.layerCount;
  return range;
}

PL_ALWAYS_INLINE vk::ImageViewType plConversionUtilsVulkan::GetImageViewType(plEnum<plGALTextureType> texType, bool bIsArrayView)
{
  switch (texType)
  {
    case plGALTextureType::Texture2D:
    case plGALTextureType::Texture2DProxy:
    case plGALTextureType::Texture2DShared:
      if (!bIsArrayView)
      {
        return vk::ImageViewType::e2D;
      }
      else
      {
        return vk::ImageViewType::e2DArray;
      }
    case plGALTextureType::TextureCube:
      if (!bIsArrayView)
      {
        return vk::ImageViewType::eCube;
      }
      else
      {
        return vk::ImageViewType::eCubeArray;
      }
    case plGALTextureType::Texture3D:
      return vk::ImageViewType::e3D;

    default:
      PL_ASSERT_NOT_IMPLEMENTED;
      return vk::ImageViewType::e1D;
  }
}

PL_ALWAYS_INLINE bool plConversionUtilsVulkan::IsDepthFormat(vk::Format format)
{
  switch (format)
  {
    case vk::Format::eD16Unorm:
    case vk::Format::eD32Sfloat:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
      return true;
    default:
      return false;
  }
}

PL_ALWAYS_INLINE bool plConversionUtilsVulkan::IsStencilFormat(vk::Format format)
{
  switch (format)
  {
    case vk::Format::eS8Uint:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
      return true;
    default:
      return false;
  }
}

PL_ALWAYS_INLINE vk::PrimitiveTopology plConversionUtilsVulkan::GetPrimitiveTopology(plEnum<plGALPrimitiveTopology> topology)
{
  switch (topology)
  {
    case plGALPrimitiveTopology::Points:
      return vk::PrimitiveTopology::ePointList;
    case plGALPrimitiveTopology::Lines:
      return vk::PrimitiveTopology::eLineList;
    case plGALPrimitiveTopology::Triangles:
      return vk::PrimitiveTopology::eTriangleList;
    default:
      PL_ASSERT_NOT_IMPLEMENTED;
      return vk::PrimitiveTopology::ePointList;
  }
}

PL_ALWAYS_INLINE vk::ShaderStageFlagBits plConversionUtilsVulkan::GetShaderStage(plGALShaderStage::Enum stage)
{
  switch (stage)
  {
    case plGALShaderStage::VertexShader:
      return vk::ShaderStageFlagBits::eVertex;
    case plGALShaderStage::HullShader:
      return vk::ShaderStageFlagBits::eTessellationControl;
    case plGALShaderStage::DomainShader:
      return vk::ShaderStageFlagBits::eTessellationEvaluation;
    case plGALShaderStage::GeometryShader:
      return vk::ShaderStageFlagBits::eGeometry;
    case plGALShaderStage::PixelShader:
      return vk::ShaderStageFlagBits::eFragment;
    default:
      PL_ASSERT_NOT_IMPLEMENTED;
      [[fallthrough]];
    case plGALShaderStage::ComputeShader:
      return vk::ShaderStageFlagBits::eCompute;
  }
}

PL_ALWAYS_INLINE vk::PipelineStageFlags plConversionUtilsVulkan::GetPipelineStage(plGALShaderStage::Enum stage)
{
  switch (stage)
  {
    case plGALShaderStage::VertexShader:
      return vk::PipelineStageFlagBits::eVertexShader;
    case plGALShaderStage::HullShader:
      return vk::PipelineStageFlagBits::eTessellationControlShader;
    case plGALShaderStage::DomainShader:
      return vk::PipelineStageFlagBits::eTessellationEvaluationShader;
    case plGALShaderStage::GeometryShader:
      return vk::PipelineStageFlagBits::eGeometryShader;
    case plGALShaderStage::PixelShader:
      return vk::PipelineStageFlagBits::eFragmentShader;
    default:
      PL_ASSERT_NOT_IMPLEMENTED;
      [[fallthrough]];
    case plGALShaderStage::ComputeShader:
      return vk::PipelineStageFlagBits::eComputeShader;
  }
}

PL_ALWAYS_INLINE vk::PipelineStageFlags plConversionUtilsVulkan::GetPipelineStage(vk::ShaderStageFlags flags)
{
  vk::PipelineStageFlags res;
  if (flags & vk::ShaderStageFlagBits::eVertex)
    res |= vk::PipelineStageFlagBits::eVertexShader;
  if (flags & vk::ShaderStageFlagBits::eTessellationControl)
    res |= vk::PipelineStageFlagBits::eTessellationControlShader;
  if (flags & vk::ShaderStageFlagBits::eTessellationEvaluation)
    res |= vk::PipelineStageFlagBits::eTessellationEvaluationShader;
  if (flags & vk::ShaderStageFlagBits::eGeometry)
    res |= vk::PipelineStageFlagBits::eGeometryShader;
  if (flags & vk::ShaderStageFlagBits::eFragment)
    res |= vk::PipelineStageFlagBits::eFragmentShader;
  if (flags & vk::ShaderStageFlagBits::eCompute)
    res |= vk::PipelineStageFlagBits::eComputeShader;

  return res;
}

PL_ALWAYS_INLINE vk::DescriptorType plConversionUtilsVulkan::GetDescriptorType(plGALShaderResourceType::Enum type)
{
  switch (type)
  {
    case plGALShaderResourceType::Unknown:
      PL_REPORT_FAILURE("Unknown descriptor type");
    case plGALShaderResourceType::Sampler:
      return vk::DescriptorType::eSampler;
    case plGALShaderResourceType::ConstantBuffer:
      return vk::DescriptorType::eUniformBuffer;
    case plGALShaderResourceType::Texture:
      return vk::DescriptorType::eSampledImage;
    case plGALShaderResourceType::TextureAndSampler:
      return vk::DescriptorType::eCombinedImageSampler;
    case plGALShaderResourceType::TexelBuffer:
      return vk::DescriptorType::eUniformTexelBuffer;
    case plGALShaderResourceType::StructuredBuffer:
      return vk::DescriptorType::eStorageBuffer;
    case plGALShaderResourceType::TextureRW:
      return vk::DescriptorType::eStorageImage;
    case plGALShaderResourceType::TexelBufferRW:
      return vk::DescriptorType::eStorageTexelBuffer;
    case plGALShaderResourceType::StructuredBufferRW:
      return vk::DescriptorType::eStorageBuffer;
  }

  return vk::DescriptorType::eMutableVALVE;
}

PL_ALWAYS_INLINE vk::ShaderStageFlagBits plConversionUtilsVulkan::GetShaderStages(plBitflags<plGALShaderStageFlags> stages)
{
  return (vk::ShaderStageFlagBits)stages.GetValue();
}

PL_ALWAYS_INLINE vk::PipelineStageFlags plConversionUtilsVulkan::GetPipelineStages(plBitflags<plGALShaderStageFlags> stages)
{
  vk::PipelineStageFlags res;
  for (int i = 0; i < plGALShaderStage::ENUM_COUNT; ++i)
  {
    plGALShaderStageFlags::Enum flag = plGALShaderStageFlags::MakeFromShaderStage((plGALShaderStage::Enum)i);
    if (stages.IsSet(flag))
    {
      res |= GetPipelineStage((plGALShaderStage::Enum)i);
    }
  }
  return res;
}

PL_CHECK_AT_COMPILETIME((plUInt32)vk::ShaderStageFlagBits::eVertex == (plUInt32)plGALShaderStageFlags::VertexShader);
PL_CHECK_AT_COMPILETIME((plUInt32)vk::ShaderStageFlagBits::eTessellationControl == (plUInt32)plGALShaderStageFlags::HullShader);
PL_CHECK_AT_COMPILETIME((plUInt32)vk::ShaderStageFlagBits::eTessellationEvaluation == (plUInt32)plGALShaderStageFlags::DomainShader);
PL_CHECK_AT_COMPILETIME((plUInt32)vk::ShaderStageFlagBits::eGeometry == (plUInt32)plGALShaderStageFlags::GeometryShader);
PL_CHECK_AT_COMPILETIME((plUInt32)vk::ShaderStageFlagBits::eFragment == (plUInt32)plGALShaderStageFlags::PixelShader);
PL_CHECK_AT_COMPILETIME((plUInt32)vk::ShaderStageFlagBits::eCompute == (plUInt32)plGALShaderStageFlags::ComputeShader);