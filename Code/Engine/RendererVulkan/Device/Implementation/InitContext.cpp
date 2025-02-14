#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Pools/CommandBufferPoolVulkan.h>
#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>


plInitContextVulkan::plInitContextVulkan(plGALDeviceVulkan* pDevice)
  : m_pDevice(pDevice)
{
  plProxyAllocator& allocator = m_pDevice->GetAllocator();
  m_pPipelineBarrier = PL_NEW(&allocator, plPipelineBarrierVulkan);
  m_pCommandBufferPool = PL_NEW(&allocator, plCommandBufferPoolVulkan);
  m_pCommandBufferPool->Initialize(m_pDevice->GetVulkanDevice(), m_pDevice->GetGraphicsQueue().m_uiQueueFamily);
  m_pStagingBufferPool = PL_NEW(&allocator, plStagingBufferPoolVulkan);
  m_pStagingBufferPool->Initialize(m_pDevice);
}

plInitContextVulkan::~plInitContextVulkan()
{
  PL_ASSERT_DEBUG(!m_currentCommandBuffer, "GetFinishedCommandBuffer should have been called before destruction.");

  m_pCommandBufferPool->DeInitialize();
  m_pStagingBufferPool->DeInitialize();
}

vk::CommandBuffer plInitContextVulkan::GetFinishedCommandBuffer()
{
  PL_LOCK(m_Lock);
  if (m_currentCommandBuffer)
  {
    m_pPipelineBarrier->Submit();
    vk::CommandBuffer res = m_currentCommandBuffer;
    res.end();

    m_pDevice->ReclaimLater(m_currentCommandBuffer, m_pCommandBufferPool.Borrow());
    return res;
  }
  return nullptr;
}

void plInitContextVulkan::EnsureCommandBufferExists()
{
  if (!m_currentCommandBuffer)
  {
    // Restart new command buffer if none is active already.
    m_currentCommandBuffer = m_pCommandBufferPool->RequestCommandBuffer();
    vk::CommandBufferBeginInfo beginInfo;
    m_currentCommandBuffer.begin(&beginInfo);
    m_pPipelineBarrier->SetCommandBuffer(&m_currentCommandBuffer);
  }
}

void plInitContextVulkan::TextureDestroyed(const plGALTextureVulkan* pTexture)
{
  PL_LOCK(m_Lock);
  m_pPipelineBarrier->TextureDestroyed(pTexture);
}

void plInitContextVulkan::InitTexture(const plGALTextureVulkan* pTexture, vk::ImageCreateInfo& createInfo, plArrayPtr<plGALSystemMemoryDescription> pInitialData)
{
  PL_LOCK(m_Lock);

  EnsureCommandBufferExists();
  if (pTexture->GetDescription().m_pExisitingNativeObject == nullptr)
  {
    if (pTexture->GetDescription().m_SampleCount != plGALMSAASampleCount::None)
    {
      // #TODO_VULKAN how do we clear a MS target to zero?
      m_pPipelineBarrier->SetInitialImageState(pTexture, vk::ImageLayout::eUndefined);
      m_pPipelineBarrier->EnsureImageLayout(pTexture, pTexture->GetPreferredLayout(), pTexture->GetUsedByPipelineStage(), pTexture->GetAccessMask(), true);
      return;
    }

    m_pPipelineBarrier->SetInitialImageState(pTexture, createInfo.initialLayout);

    plDynamicArray<plUInt8> tempData;
    plDynamicArray<plGALSystemMemoryDescription> initialData;
    if (pInitialData.IsEmpty())
    {
      // If we don't have any initial data to initialize the texture to zero memory to match DX11 behavior.
      const vk::Format format = pTexture->GetImageFormat();
      const plUInt8 uiBlockSize = vk::blockSize(format);
      const auto blockExtent = vk::blockExtent(format);
      {
        // Compute max size of the temp buffer.
        const vk::Extent3D imageExtent = pTexture->GetMipLevelSize(0);
        const VkExtent3D blockCount = {
          (imageExtent.width + blockExtent[0] - 1) / blockExtent[0],
          (imageExtent.height + blockExtent[1] - 1) / blockExtent[1],
          (imageExtent.depth + blockExtent[2] - 1) / blockExtent[2]};
        const plUInt32 uiTotalSize = uiBlockSize * blockCount.width * blockCount.height * blockCount.depth;
        tempData.SetCount(uiTotalSize, 0);
      }

      for (plUInt32 uiLayer = 0; uiLayer < createInfo.arrayLayers; uiLayer++)
      {
        for (plUInt32 uiMipLevel = 0; uiMipLevel < createInfo.mipLevels; uiMipLevel++)
        {
          const plUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * createInfo.mipLevels;
          PL_ASSERT_DEBUG(initialData.GetCount() == uiSubresourceIndex, "");

          const vk::Extent3D imageExtent = pTexture->GetMipLevelSize(uiMipLevel);
          const VkExtent3D blockCount = {
            (imageExtent.width + blockExtent[0] - 1) / blockExtent[0],
            (imageExtent.height + blockExtent[1] - 1) / blockExtent[1],
            (imageExtent.depth + blockExtent[2] - 1) / blockExtent[2]};

          plGALSystemMemoryDescription data;
          data.m_pData = tempData.GetData();
          data.m_uiRowPitch = uiBlockSize * blockCount.width;
          data.m_uiSlicePitch = data.m_uiRowPitch * blockCount.height;
          initialData.PushBack(data);
        }
      }
      pInitialData = initialData.GetArrayPtr();
    }

    m_pPipelineBarrier->EnsureImageLayout(pTexture, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite);
    // We need to flush here as all UploadTextureStaging calls will need to barrier on eTransferDstOptimal anyways.
    m_pPipelineBarrier->Flush();
    for (plUInt32 uiLayer = 0; uiLayer < createInfo.arrayLayers; uiLayer++)
    {
      for (plUInt32 uiMipLevel = 0; uiMipLevel < createInfo.mipLevels; uiMipLevel++)
      {
        const plUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * createInfo.mipLevels;
        PL_ASSERT_DEBUG(uiSubresourceIndex < pInitialData.GetCount(), "Not all data provided in the intial texture data.");
        const plGALSystemMemoryDescription& subResourceData = pInitialData[uiSubresourceIndex];

        vk::ImageSubresourceLayers subresourceLayers;
        // We do not support stencil uploads right now.
        subresourceLayers.aspectMask = plConversionUtilsVulkan::IsDepthFormat(pTexture->GetImageFormat()) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
        subresourceLayers.mipLevel = uiMipLevel;
        subresourceLayers.baseArrayLayer = uiLayer;
        subresourceLayers.layerCount = 1;

        m_pDevice->UploadTextureStaging(m_pStagingBufferPool.Borrow(), m_pPipelineBarrier.Borrow(), m_currentCommandBuffer, pTexture, subresourceLayers, subResourceData);
      }
    }
  }
  else
  {
    // We don't actually know what the current state is of an existing native object. The only use case right now are back buffers created by swap chains so for now we just throw away the current content and barrier into the preferred layout.
    m_pPipelineBarrier->SetInitialImageState(pTexture, vk::ImageLayout::eUndefined);
    m_pPipelineBarrier->EnsureImageLayout(pTexture, pTexture->GetPreferredLayout(), pTexture->GetUsedByPipelineStage(), pTexture->GetAccessMask(), true);
  }
}
