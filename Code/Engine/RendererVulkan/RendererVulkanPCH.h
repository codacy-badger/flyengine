#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif


#define VULKAN_HPP_NO_NODISCARD_WARNINGS // TODO: temporarily disable warnings to make it compile. Need to fix all the warnings later.

#if PL_ENABLED(PL_PLATFORM_LINUX)
#  define VK_USE_PLATFORM_XCB_KHR
#endif

#include <vulkan/vulkan.hpp>
// Some of the functionality we need has moved from vulkan.hpp to vulkan_format_traits.hpp in later versions of the vulkan SDK.
#if __has_include(<vulkan/vulkan_format_traits.hpp>)
#  include <vulkan/vulkan_format_traits.hpp>
#endif

#if VK_HEADER_VERSION < 211
#  error "Your vulkan headers are to old. The headers of SDK 1.3.211 or newer are required"
#endif

#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
