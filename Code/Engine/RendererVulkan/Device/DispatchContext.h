#pragma once

#include <vulkan/vulkan.h>

class plGALDeviceVulkan;

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
#  define PL_DISPATCH_CONTEXT_MEMBER_NAME(Name) m_p##Name
#else
#  define PL_DISPATCH_CONTEXT_MEMBER_NAME(Name) Name
#endif

// A vulkan hpp compatible dispatch context.
class plVulkanDispatchContext
{
public:
  void Init(plGALDeviceVulkan& device);

  plUInt32 getVkHeaderVersion() const { return VK_HEADER_VERSION; }

#if PL_ENABLED(PL_PLATFORM_LINUX)
  // VK_KHR_external_memory_fd
  PFN_vkGetMemoryFdKHR PL_DISPATCH_CONTEXT_MEMBER_NAME(vkGetMemoryFdKHR) = nullptr;
  PFN_vkGetMemoryFdPropertiesKHR PL_DISPATCH_CONTEXT_MEMBER_NAME(vkGetMemoryFdPropertiesKHR) = nullptr;

  // VK_KHR_external_semaphore_fd
  PFN_vkGetSemaphoreFdKHR PL_DISPATCH_CONTEXT_MEMBER_NAME(vkGetSemaphoreFdKHR) = nullptr;
  PFN_vkImportSemaphoreFdKHR PL_DISPATCH_CONTEXT_MEMBER_NAME(vkImportSemaphoreFdKHR) = nullptr;
#elif PL_ENABLED(PL_PLATFORM_WINDOWS)
  // VK_KHR_external_memory_win32
  PFN_vkGetMemoryWin32HandleKHR PL_DISPATCH_CONTEXT_MEMBER_NAME(vkGetMemoryWin32HandleKHR) = nullptr;
  PFN_vkGetMemoryWin32HandlePropertiesKHR PL_DISPATCH_CONTEXT_MEMBER_NAME(vkGetMemoryWin32HandlePropertiesKHR) = nullptr;

  // VK_KHR_external_semaphore_win32
  PFN_vkGetSemaphoreWin32HandleKHR PL_DISPATCH_CONTEXT_MEMBER_NAME(vkGetSemaphoreWin32HandleKHR) = nullptr;
  PFN_vkImportSemaphoreWin32HandleKHR PL_DISPATCH_CONTEXT_MEMBER_NAME(vkImportSemaphoreWin32HandleKHR) = nullptr;
#endif

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
#  if PL_ENABLED(PL_PLATFORM_LINUX)
  // VK_KHR_external_memory_fd
  VkResult vkGetMemoryFdKHR(VkDevice device, const VkMemoryGetFdInfoKHR* pGetFdInfo, int* pFd) const;
  VkResult vkGetMemoryFdPropertiesKHR(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, int fd, VkMemoryFdPropertiesKHR* pMemoryFdProperties) const;

  // VK_KHR_external_semaphore_fd
  VkResult vkGetSemaphoreFdKHR(VkDevice device, const VkSemaphoreGetFdInfoKHR* pGetFdInfo, int* pFd) const;
  VkResult vkImportSemaphoreFdKHR(VkDevice device, const VkImportSemaphoreFdInfoKHR* pImportSemaphoreFdInfo) const;
#  elif PL_ENABLED(PL_PLATFORM_WINDOWS)
  // VK_KHR_external_memory_win32
  VkResult vkGetMemoryWin32HandleKHR(VkDevice device, const VkMemoryGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pWin32Handle) const;
  VkResult vkGetMemoryWin32HandlePropertiesKHR(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, HANDLE Win32Handle, VkMemoryWin32HandlePropertiesKHR* pMemoryWin32HandleProperties) const;

  // VK_KHR_external_semaphore_win32
  VkResult vkGetSemaphoreWin32HandleKHR(VkDevice device, const VkSemaphoreGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pWin32Handle) const;
  VkResult vkImportSemaphoreWin32HandleKHR(VkDevice device, const VkImportSemaphoreWin32HandleInfoKHR* pImportSemaphoreWin32HandleInfo) const;
#  endif
#endif

private:
  plGALDeviceVulkan* m_pDevice = nullptr;
};
