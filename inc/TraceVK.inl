// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : TraceVK.inl
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 13d
// * Last Altered: 2019y 09m 13d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * Description :
// *    This is an inline file that describes insertion operator overloads
// *  for Trace to be able to print out Vulkan structures.
// *
// *
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#ifndef DW_TRACE_VK_INL
#define DW_TRACE_VK_INL

#include "Trace.h"
#include <vulkan/vulkan.h>
#include <vector>

template <>
inline Trace& operator<<<VkExtent3D>(Trace& t, VkExtent3D const& e) {
  t << "[w:" << e.width << ", h:" << e.height << ", d:" << e.depth << "]";

  return t;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// PHYSICAL DEVICE
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <>
inline Trace& operator<<<VkPhysicalDeviceFeatures>(Trace& tr, VkPhysicalDeviceFeatures const& m) {
  const char* names[] = {
    "robustBufferAccess",
    "fullDrawIndexUint32",
    "imageCubeArray",
    "independentBlend",
    "geometryShader",
    "tessellationShader",
    "sampleRateShading",
    "dualSrcBlend",
    "logicOp",
    "multiDrawIndirect",
    "drawIndirectFirstInstance",
    "depthClamp",
    "depthBiasClamp",
    "fillModeNonSolid",
    "depthBounds",
    "wideLines",
    "largePoints",
    "alphaToOne",
    "multiViewport",
    "samplerAnisotropy",
    "textureCompressionETC2",
    "textureCompressionASTC_LDR",
    "textureCompressionBC",
    "occlusionQueryPrecise",
    "pipelineStatisticsQuery",
    "vertexPipelineStoresAndAtomics",
    "fragmentStoresAndAtomics",
    "shaderTessellationAndGeometryPointSize",
    "shaderImageGatherExtended",
    "shaderStorageImageExtendedFormats",
    "shaderStorageImageMultisample",
    "shaderStorageImageReadWithoutFormat",
    "shaderStorageImageWriteWithoutFormat",
    "shaderUniformBufferArrayDynamicIndexing",
    "shaderSampledImageArrayDynamicIndexing",
    "shaderStorageBufferArrayDynamicIndexing",
    "shaderStorageImageArrayDynamicIndexing",
    "shaderClipDistance",
    "shaderCullDistance",
    "shaderFloat64",
    "shaderInt64",
    "shaderInt16",
    "shaderResourceResidency",
    "shaderResourceMinLod",
    "sparseBinding",
    "sparseResidencyBuffer",
    "sparseResidencyImage2D",
    "sparseResidencyImage3D",
    "sparseResidency2Samples",
    "sparseResidency4Samples",
    "sparseResidency8Samples",
    "sparseResidency16Samples",
    "sparseResidencyAliased",
    "variableMultisampleRate",
    "inheritedQueries"
  };

  for (uint32_t i = 0; i < sizeof(m) / sizeof(VkBool32); ++i) {
    tr << (names[i]) << ": " << static_cast<bool>(*(reinterpret_cast<const VkBool32*>(&m) + i)) << "\n";
  }

  return tr;
}

template <>
inline Trace& operator<<<VkPhysicalDeviceMemoryProperties>(Trace& t, VkPhysicalDeviceMemoryProperties const& m) {
  t << "Heap Count: " << m.memoryHeapCount << "\n Device Local | Multi Instance\n";
  for (int i = 0; i < m.memoryHeapCount; ++i)
    t << "  #" << i << ": "
        << ((m.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? " X " : " n ")
        << ((m.memoryHeaps[i].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) ? " X " : " n ")
        << "\n";

  t << "Memory Type Count: " << m.memoryTypeCount << "\n Dev | Vis | Coh | Cach | Lazy\n";
  for (int i = 0; i < m.memoryTypeCount; ++i)
    t << "  #" << i << ": "
        << ((m.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ? " X " : " n ")
        << ((m.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ? " X " : " n ")
        << ((m.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ? " X " : " n ")
        << ((m.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) ? " X " : " n ")
        << ((m.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) ? " X " : " n ")
        << "\n";

  return t;
}

template <>
inline Trace& operator<<<VkPhysicalDeviceProperties>(Trace& t, VkPhysicalDeviceProperties const& m) {
  t << "Device Name: " << m.deviceName << "\n"
    << "Device Type: " << m.deviceType << "\n"
    << "Driver Vers: " << m.driverVersion << "\n"
    << "API Version: " << m.apiVersion << "\n";

  return t;
}

template <>
inline Trace& operator<<<VkQueueFamilyProperties>(Trace& t, VkQueueFamilyProperties const& m) {
  t << "    Queue Count: " << m.queueCount << "\n"
    << "    Queue Flags: GR: "
    << ((m.queueFlags & VK_QUEUE_GRAPHICS_BIT) ? "X " : "  ") << "| "
    << "CP: " << ((m.queueFlags & VK_QUEUE_COMPUTE_BIT) ? "X " : "  ") << "| "
    << "TR: " << (m.queueFlags & VK_QUEUE_TRANSFER_BIT ? "X " : "  ") << "| "
    << "SB: " << (m.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT ? "X " : "  ") << "| "
    << "PR: " << (m.queueFlags & VK_QUEUE_PROTECTED_BIT ? "X " : "  ") << "\n"
    << "    Min Image Transfer Gran.:" << m.minImageTransferGranularity << "\n";

  return t;
}

#endif
