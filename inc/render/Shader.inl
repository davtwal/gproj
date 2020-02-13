// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Shader.inl
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

#ifndef DW_SHADER_INL
#define DW_SHADER_INL

#include "Shader.h"

namespace dw {
  template <ShaderStage TStage>
  Shader<TStage>::Shader(Shader&& o) noexcept
    : IShader(std::move(o)) {
    
  }

  template <ShaderStage TStage>
  Shader<TStage>::Shader(ShaderModule&& mod)
    : IShader(std::move(mod)) {
    
  }

  template <ShaderStage TStage>
  VkPipelineShaderStageCreateInfo Shader<TStage>::getCreateInfo() const {
    VkPipelineShaderStageCreateInfo ret = {};
    return ret;
  }

#define CREATE_GET_CREATE_INFO_FOR_SHADER(stageName, stageBit)  \
  template <>                                                   \
  inline VkPipelineShaderStageCreateInfo                        \
  Shader<ShaderStage::##stageName##>::getCreateInfo() const {   \
    VkPipelineShaderStageCreateInfo ret = {                     \
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,      \
      nullptr,                                                  \
      0,                                                        \
      stageBit,                                                 \
      m_module,                                                 \
      getEntryPoint(),                                          \
      nullptr                                                   \
    };                                                          \
                                                                \
    return ret;                                                 \
  }

  CREATE_GET_CREATE_INFO_FOR_SHADER(Vertex, VK_SHADER_STAGE_VERTEX_BIT);
  CREATE_GET_CREATE_INFO_FOR_SHADER(Hull, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
  CREATE_GET_CREATE_INFO_FOR_SHADER(Domain, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
  CREATE_GET_CREATE_INFO_FOR_SHADER(Geometry, VK_SHADER_STAGE_GEOMETRY_BIT);
  CREATE_GET_CREATE_INFO_FOR_SHADER(Fragment, VK_SHADER_STAGE_FRAGMENT_BIT);
  CREATE_GET_CREATE_INFO_FOR_SHADER(Compute, VK_SHADER_STAGE_COMPUTE_BIT);

#undef CREATE_GET_CREATE_INFO_FOR_SHADER

#ifdef DW_SHADER_ALLOW_NON_MAIN_ENTRY
  template <ShaderStage TStage>
  Shader<TStage>::Shader(ShaderModule&& mod, std::string const& entryPoint)
    : m_module(std::move(mod)), m_entryPoint(entryPoint) {
  }

#endif
}

#endif
