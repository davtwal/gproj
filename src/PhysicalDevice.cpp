// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : PhysicalDevice.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 13d
// * Last Altered: 2019y 09m 13d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * Description :
// *
// *
// *
// *
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#include "PhysicalDevice.h"
#include <cassert>
#include "Trace.h"

namespace dw {
  PhysicalDevice::PhysicalDevice(VulkanControl& vkctrl, VkPhysicalDevice dev)
    : m_control(vkctrl), m_device(dev) {
    assert(dev);
  }

  PhysicalDevice::operator VkPhysicalDevice() const {
    return m_device;
  }

  std::vector<std::string> const& PhysicalDevice::getAvailableLayers() const {
    return m_layers;
  }

  std::vector<std::string> const& PhysicalDevice::getAvailableExtensions() const {
    return m_extensions;
  }

  VkPhysicalDeviceFeatures const& PhysicalDevice::getFeatures() const {
    return m_features;
  }

  VkPhysicalDeviceMemoryProperties const& PhysicalDevice::getMemoryProperties() const {
    return m_memProps;
  }

  std::vector<VkQueueFamilyProperties> const& PhysicalDevice::getQueueFamilyProperties() const {
    return m_queueFamilies;
  }
  
  VkFormatProperties PhysicalDevice::getFormatProperties(VkFormat format) const {
    VkFormatProperties ret = {};
    vkGetPhysicalDeviceFormatProperties(m_device, format, &ret);
    return ret;
  }

  void PhysicalDevice::queryLayers() {
    uint32_t numLayers = 0;
    vkEnumerateDeviceLayerProperties(m_device, &numLayers, nullptr);

    if (numLayers) {
      std::vector<VkLayerProperties> layers;
      layers.resize(numLayers);
      vkEnumerateDeviceLayerProperties(m_device, &numLayers, layers.data());

      m_layers.reserve(numLayers);
      for (auto& layer : layers) {
        m_layers.emplace_back(layer.layerName);
      }
    }
  }

  void PhysicalDevice::queryExtensions() {
    uint32_t numExt = 0;
    vkEnumerateDeviceExtensionProperties(m_device, nullptr, &numExt, nullptr);

    if (numExt) {
      std::vector<VkExtensionProperties> exts;
      exts.resize(numExt);
      vkEnumerateDeviceExtensionProperties(m_device, nullptr, &numExt, exts.data());

      m_layers.reserve(numExt);
      for (auto& ext : exts) {
        m_layers.emplace_back(ext.extensionName);
      }
    }
  }

  void PhysicalDevice::queryQueueFamilies() {
    uint32_t numExt = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_device, &numExt, nullptr);

    if (numExt) {
      std::vector<VkQueueFamilyProperties> exts;
      m_queueFamilies.resize(numExt);
      vkGetPhysicalDeviceQueueFamilyProperties(m_device, &numExt, m_queueFamilies.data());
    }
  }

  uint32_t PhysicalDevice::pickMemoryType(VkMemoryPropertyFlagBits memoryRequired,
                                          VkFlags                  required,
                                          VkFlags                  preferred) const {
    for (uint32_t i = 0; i < m_memProps.memoryTypeCount; ++i) {
      if ((m_memProps.memoryTypes[i].propertyFlags & memoryRequired) == preferred)
        return i;
    }

    for (uint32_t i = 0; i < m_memProps.memoryTypeCount; ++i) {
      if ((m_memProps.memoryTypes[i].propertyFlags & memoryRequired) == required)
        return i;
    }

    return std::numeric_limits<uint32_t>::max();
  }


  uint32_t PhysicalDevice::pickQueueFamily(VkQueueFlagBits flag) {
    // try to find a queue family that matches exactly first
    for (uint32_t i = 0; i < m_queueFamilies.size(); ++i) {
      if (m_queueFamilies[i].queueFlags == flag)
        return i;
    }

    for (uint32_t i = 0; i < m_queueFamilies.size(); ++i) {
      if ((m_queueFamilies[i].queueFlags & flag) == flag)
        return i;
    }

    return std::numeric_limits<uint32_t>::max();
  }

  void PhysicalDevice::gatherData() {
    vkGetPhysicalDeviceProperties(m_device, &m_properties);
    vkGetPhysicalDeviceFeatures(m_device, &m_features);
    vkGetPhysicalDeviceMemoryProperties(m_device, &m_memProps);
    // query extensions
    queryLayers();
    queryExtensions();
    queryQueueFamilies();
    //vkGetPhysicalDeviceFormatProperties(m_device, );

#ifdef _DEBUG
    Trace::All << "Physical Device " << m_properties.deviceName << " registered: " << Trace::Stop;
    Trace::All << m_properties << Trace::Stop;
    Trace::All << m_features << Trace::Stop;
    Trace::All << m_memProps << Trace::Stop;
    Trace::All << "Queue Families: \n" << m_queueFamilies << Trace::Stop;
    Trace::All << m_extensions << Trace::Stop;
    Trace::All << m_layers << Trace::Stop;
#endif

    //vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device, );
    //vkGetPhysicalDeviceSurfaceFormatsKHR(m_device, );
  }

  const VkPhysicalDeviceLimits& PhysicalDevice::getLimits() const {
    return m_properties.limits;
  }

}
