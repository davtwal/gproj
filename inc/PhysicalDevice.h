// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : PhysicalDevice.h
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

#ifndef DW_PHYSICAL_DEVICE_H
#define DW_PHYSICAL_DEVICE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

#include "VulkanControl.h"

#define PHYSICAL_DEPENDENT_FUNCTION(varName, x) \
  NO_DISCARD PhysicalDevice& getOwningPhysical() const {return (varName).getOwningPhysical();} \
  CONTROL_DEPENDENT_FUNCTION(varName, x)

#define CREATE_PHYSICAL_DEPENDENT(x)                                              \
  class x {                                                                       \
    PhysicalDevice& m_physical;                                                      \
  public:                                                                         \
    NO_DISCARD PhysicalDevice& getOwningPhysical() const {return m_physical;}                   \
    CONTROL_DEPENDENT_FUNCTION(m_physical, x) \
  private:

#define CREATE_PHYSICAL_DEPENDENT_INHERIT(x, ...)                                 \
  class x : __VA_ARGS__ {                                                         \
    PhysicalDevice& m_physical;                                                      \
  public:                                                                         \
    NO_DISCARD PhysicalDevice& getOwningPhysical() const {return m_physical;}                   \
    CONTROL_DEPENDENT_FUNCTION(m_physical, x) \
  private:

namespace dw {
  class VulkanControl;

  CREATE_CONTROL_DEPENDENT(PhysicalDevice)
  public:
    PhysicalDevice(VulkanControl& vkctrl, VkPhysicalDevice dev);

    NO_DISCARD std::vector<std::string> const& getAvailableExtensions() const;
    NO_DISCARD std::vector<std::string> const& getAvailableLayers() const;
    NO_DISCARD const VkPhysicalDeviceLimits& getLimits() const;

    NO_DISCARD uint32_t pickMemoryType(VkMemoryPropertyFlagBits memoryRequired,
                                          VkFlags                  required,
                                          VkFlags                  preferred) const;
    NO_DISCARD uint32_t pickQueueFamily(VkQueueFlagBits flag);

    operator VkPhysicalDevice() const;

    NO_DISCARD VkPhysicalDeviceFeatures const& getFeatures() const;
    NO_DISCARD VkPhysicalDeviceMemoryProperties const& getMemoryProperties() const;
    NO_DISCARD std::vector<VkQueueFamilyProperties> const& getQueueFamilyProperties() const;

    NO_DISCARD VkFormatProperties getFormatProperties(VkFormat format) const;

  private:
    friend class VulkanControl;
    friend class LogicalDevice;

    void gatherData();
    void queryExtensions();
    void queryLayers();
    void queryQueueFamilies();

    VkPhysicalDevice m_device{nullptr};

    VkPhysicalDeviceProperties       m_properties{};
    VkPhysicalDeviceFeatures         m_features{};
    VkPhysicalDeviceMemoryProperties m_memProps{};

    std::vector<VkQueueFamilyProperties> m_queueFamilies;

    std::vector<std::string> m_extensions;
    std::vector<std::string> m_layers;
  };
}

#endif
