// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : Surface.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 16d
// * Last Altered: 2019y 09m 16d
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

#include "Surface.h"
#include "VulkanControl.h"
#include <algorithm>

namespace dw {
  Surface::Surface(GLFWWindow& window, PhysicalDevice& device)
    : m_window(window), m_physical(device) {
    glfwCreateWindowSurface(m_physical.getOwningControl(), m_window.getHandle(), nullptr, &m_surface);

    if (!m_surface)
      throw std::bad_alloc();

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR (m_physical, m_surface, &m_capabilities);

    queryFormats();
    queryPresentModes();
  }

  VkSurfaceCapabilitiesKHR const& Surface::getCapabilities() const {
    return m_capabilities;
  }

  std::vector<VkSurfaceFormatKHR> const& Surface::getFormats() const {
    return m_formats;
  }

  std::vector<VkPresentModeKHR> const& Surface::getPresentModes() const {
    return m_presentModes;
  }

  Surface::operator VkSurfaceKHR() const {
    return m_surface;
  }

  uint32_t Surface::getHeight() const {
    return m_window.getHeight();
  }

  uint32_t Surface::getWidth() const {
    return m_window.getWidth();
  }

  VkSurfaceFormatKHR const& Surface::chooseFormat() const {
    for(auto& format : m_formats) {
      if (format.format == VK_FORMAT_R8G8B8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        return format;
    }

    return m_formats.front();
  }

  VkPresentModeKHR Surface::choosePresentMode(bool vsync) const {
    for(auto& mode : m_presentModes) {
      if (mode == (vsync ? VK_PRESENT_MODE_FIFO_RELAXED_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR))
        return mode;
    }

    // only FIFO is guaranteed to be available by the standard
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D Surface::chooseExtent() const {
    if (m_capabilities.currentExtent.width != UINT32_MAX) {
      return m_capabilities.currentExtent;
    }
    else {
      VkExtent2D actualExtent = { getWidth(), getHeight() };

      actualExtent.width = std::max(m_capabilities.minImageExtent.width, std::min(m_capabilities.maxImageExtent.width, actualExtent.width));
      actualExtent.height = std::max(m_capabilities.minImageExtent.height, std::min(m_capabilities.maxImageExtent.height, actualExtent.height));

      return actualExtent;
    }
  }

  void Surface::queryFormats() {
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical, m_surface, &formatCount, nullptr);

    if(formatCount) {
      m_formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical, m_surface, &formatCount, m_formats.data());
    }
  }

  void Surface::queryPresentModes() {
    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical, m_surface, &modeCount, nullptr);

    if (modeCount) {
      m_presentModes.resize(modeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical, m_surface, &modeCount, m_presentModes.data());
    }
  }

}
