// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : VulkanControl.cpp
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

#include "render/VulkanControl.h"
#include "render/PhysicalDevice.h"
#include "util/Trace.h"

namespace dw {
  std::vector<std::string> VulkanControl::s_availableExts;
  std::vector<std::string> VulkanControl::s_availableLayers;

  VulkanControl::VulkanControl(std::string const& appName, uint32_t vkVersion, uint32_t appVersion)
    : m_app_name(appName),
      m_app_info({
                   VK_STRUCTURE_TYPE_APPLICATION_INFO,
                   nullptr,
                   m_app_name.c_str(),
                   appVersion,
                   m_app_name.c_str(),
                   appVersion,
                   vkVersion
                 }) {
    Trace::All << "Created VulkanControl with name " << appName
        << ", VK version " << vkVersion
        << ", and appVersion " << appVersion
        << Trace::Stop;

    GetAvailableExtensions();
    GetAvailableLayers();
  }

  VulkanControl::~VulkanControl() {
    if (m_instance)
      vkDestroyInstance(m_instance, nullptr);
    else
      Trace::Warn << "Attempted to shut-down VulkanControl that didn't have an instance" << Trace::Stop;
  }

  std::vector<std::string> const& VulkanControl::GetAvailableExtensions() {
    if (s_availableExts.empty()) {
      uint32_t extCount = 0;
      vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);

      std::vector<VkExtensionProperties> exts;
      if (extCount) {
        exts.resize(extCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extCount, exts.data());

        s_availableExts.reserve(extCount);
        for (auto& ext : exts)
          s_availableExts.emplace_back(ext.extensionName);
      }

      Trace::All << "Evaluated extensions." << Trace::Stop;
    }

    return s_availableExts;
  }

  std::vector<std::string> const& VulkanControl::GetAvailableLayers() {
    if (s_availableLayers.empty()) {
      uint32_t layerCount = 0;
      vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

      if (layerCount) {
        std::vector<VkLayerProperties> layers;
        layers.resize(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

        s_availableLayers.reserve(layerCount);
        for (auto& layer : layers)
          s_availableLayers.emplace_back(layer.layerName);

        Trace::All << "Evaluated layers." << Trace::Stop;
      }
    }

    return s_availableLayers;
  }

  bool VulkanControl::IsExtensionAvailable(const char* ext) {
    return std::find(s_availableExts.begin(), s_availableExts.end(), std::string(ext)) != s_availableExts.end();
  }

  bool VulkanControl::IsLayerAvailable(const char* layer) {
    return std::find(s_availableLayers.begin(), s_availableLayers.end(), std::string(layer)) != s_availableLayers.end();
  }

  bool VulkanControl::initInstance(std::vector<const char*> const& extensions, std::vector<const char*> const& layers) {
    VkInstanceCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      nullptr,
      0,
      &m_app_info,
      static_cast<uint32_t>(layers.size()),
      layers.data(),
      static_cast<uint32_t>(extensions.size()),
      extensions.data()
    };

#ifdef _DEBUG
    Trace::All << "VulkanControl: Creating instance with "
        << extensions.size() << " extension(s) and " << layers.size() << " layer(s):\nACTIVE EXTENSIONS:\n";

    for (const char* ext : extensions)
      Trace::All << " - " << ext << "\n";

    Trace::All << "ACTIVE LAYERS:\n";
    for (const char* layer : layers)
      Trace::All << " - " << layer << "\n";

    Trace::All << Trace::Stop;
#endif

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
      return false;

    return m_instance != nullptr;
  }

  void VulkanControl::registerPhysicalDevices() {
    uint32_t physDevCount = 0;

    vkEnumeratePhysicalDevices(m_instance, &physDevCount, nullptr);

    if (physDevCount) {
      std::vector<VkPhysicalDevice> physDevices;
      physDevices.resize(physDevCount);
      m_phys_devices.reserve(physDevCount);

      vkEnumeratePhysicalDevices(m_instance, &physDevCount, physDevices.data());
      for (auto& dev : physDevices) {
        m_phys_devices.emplace_back(*this, dev).gatherData();
      }
    }
  }

  std::vector<PhysicalDevice> const& VulkanControl::getPhysicalDevices() const {
    return m_phys_devices;
  }

  std::vector<PhysicalDevice>& VulkanControl::getPhysicalDevices() {
    return m_phys_devices;
  }


  VulkanControl::operator VkInstance() const {
    return m_instance;
  }

  PhysicalDevice& VulkanControl::getBestPhysical() {
    return m_phys_devices.front();
  }
}
