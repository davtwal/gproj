// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : Surface.h
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

#ifndef DW_VK_SURFACE_H
#define DW_VK_SURFACE_H

#include "GLFWWindow.h"
#include "PhysicalDevice.h"

namespace dw {
  class GLFWWindow;

  CREATE_PHYSICAL_DEPENDENT(Surface)
  public:
    Surface(GLFWWindow& window, PhysicalDevice& device);

    operator VkSurfaceKHR() const;

    NO_DISCARD VkSurfaceCapabilitiesKHR const& getCapabilities() const;
    NO_DISCARD std::vector<VkSurfaceFormatKHR> const& getFormats() const;
    NO_DISCARD std::vector<VkPresentModeKHR> const& getPresentModes() const;
    NO_DISCARD GLFWWindow& getOwningWindow() const { return m_window; }

    NO_DISCARD uint32_t getWidth() const;
    NO_DISCARD uint32_t getHeight() const;

    NO_DISCARD VkPresentModeKHR choosePresentMode(bool vsync = false) const;
    NO_DISCARD VkSurfaceFormatKHR const& chooseFormat() const;
    NO_DISCARD VkExtent2D chooseExtent() const;

  private:
    GLFWWindow& m_window;
    VkSurfaceKHR m_surface{ nullptr };

    VkSurfaceCapabilitiesKHR m_capabilities{};

    std::vector<VkSurfaceFormatKHR> m_formats;
    std::vector<VkPresentModeKHR> m_presentModes;

    void queryFormats();
    void queryPresentModes();
  };
}

#endif
