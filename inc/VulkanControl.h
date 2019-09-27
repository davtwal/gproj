// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : VulkanControl.h
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

#ifndef DW_VULKAN_CONTROL_H
#define DW_VULKAN_CONTROL_H

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#ifndef NO_DISCARD
  #define NO_DISCARD [[nodiscard]]
#endif

#define MOVE_CONSTRUCT_ONLY(className) public:          \
  className ( className && o) noexcept;                 \
  className ( className const&) = delete;               \
  className & operator=( className && o) = delete;      \
  className & operator=( className const &) = delete;
  

#define CONTROL_DEPENDENT_FUNCTION(varName, className)                                        \
  NO_DISCARD VulkanControl& getOwningControl() const { return (varName).getOwningControl(); } \
  MOVE_CONSTRUCT_ONLY(className)

#define CREATE_CONTROL_DEPENDENT(x)                                         \
  class x {                                                                 \
    VulkanControl& m_control;                                               \
  public:                                                                   \
    NO_DISCARD VulkanControl& getOwningControl() const {return m_control;}  \
  private:

#define CREATE_CONTROL_DEPENDENT_INHERIT(x, ...)                            \
  class x : __VA_ARGS__ {                                                   \
    VulkanControl& m_control;                                               \
  public:                                                                   \
    NO_DISCARD VulkanControl& getOwningControl() const {return m_control;}  \
  private:

namespace dw {
  class PhysicalDevice;

  class VulkanControl {
  public:
    static constexpr const char* LAYER_STANDARD_VALIDATION = "VK_LAYER_LUNARG_standard_validation";
    static constexpr const char* LAYER_RENDERDOC_CAPTURE   = "VK_LAYER_RENDERDOC_Capture";

    static constexpr const char* EXT_DEBUG_REPORT = "VK_EXT_debug_report";
    static constexpr const char* EXT_DEBUG_UTILS = "VK_EXT_debug_utils";

    static constexpr const char* DEVEXT_SWAPCHAIN = "VK_KHR_swapchain";

    VulkanControl(std::string const& appName, uint32_t vkVersion, uint32_t appVersion);
    ~VulkanControl();

    static std::vector<std::string> const& GetAvailableExtensions();
    static std::vector<std::string> const& GetAvailableLayers();
    static bool                            IsExtensionAvailable(const char* ext);
    static bool                            IsLayerAvailable(const char* layer);

    bool initInstance(std::vector<const char*> const& extensions = {}, std::vector<const char*> const& layers = {});
    void registerPhysicalDevices();

    NO_DISCARD PhysicalDevice& getBestPhysical();
    NO_DISCARD std::vector<PhysicalDevice> const& getPhysicalDevices() const;
    NO_DISCARD std::vector<PhysicalDevice>& getPhysicalDevices();

    operator VkInstance() const;

  private:
    static std::vector<std::string> s_availableExts;
    static std::vector<std::string> s_availableLayers;

    const std::string       m_app_name;
    const VkApplicationInfo m_app_info;
    VkInstance              m_instance{nullptr};

    std::vector<PhysicalDevice> m_phys_devices;
  };


  class ControlDependent {
  public:
    NO_DISCARD VulkanControl& getOwningControl() const {
      return m_control;
    }

  protected:
    VulkanControl& m_control;
  };
}

#endif
