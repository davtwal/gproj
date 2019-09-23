// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Shader.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 23d
// * Last Altered: 2019y 09m 23d
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

#include "Shader.h"

#include <fstream>

namespace dw {
  ShaderModule::ShaderModule(LogicalDevice& device, std::vector<char> const& spirv_binary)
    : m_device(device) {

    VkShaderModuleCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      nullptr,
      0,
      spirv_binary.size(),
      reinterpret_cast<const uint32_t*>(spirv_binary.data())
    };
    
    vkCreateShaderModule(m_device, &createInfo, nullptr, &m_module);
  }

  ShaderModule::ShaderModule(ShaderModule&& o) noexcept
    : m_device(o.m_device),
      m_module(o.m_module) {
    o.m_module = nullptr;
  }

  ShaderModule::~ShaderModule() {
    if (m_module)
      vkDestroyShaderModule(m_device, m_module, nullptr);
  }

  ShaderModule ShaderModule::Load(LogicalDevice& device, std::string const& filename) {
    std::fstream file;

    file.open(filename, std::ios_base::ate | std::ios_base::binary);
    if(!file.is_open()) {
      throw std::runtime_error("Could not open file " + filename + " for read");
    }

    const auto fileSize = file.tellg();

    file.seekg(0);

    std::vector<char> shaderStr(fileSize);
    file.read(shaderStr.data(), fileSize);

    return ShaderModule(device, shaderStr);
  }

  ShaderModule::operator VkShaderModule() const {
    return m_module;
  }

}