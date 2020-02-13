// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Shader.h
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

#ifndef DW_SHADER_H
#define DW_SHADER_H

#include "LogicalDevice.h"

namespace dw {
  CREATE_DEVICE_DEPENDENT(ShaderModule) 
  public:
    ShaderModule(LogicalDevice& device, std::vector<char> const& spirv_binary);
    ~ShaderModule();

    static ShaderModule Load(LogicalDevice& device, std::string const& filename);

    operator VkShaderModule() const;

  private:
    VkShaderModule m_module{ nullptr };
  };

  class IShader {
  public:
    IShader(ShaderModule&& mod);
    virtual ~IShader() {}

    NO_DISCARD virtual VkPipelineShaderStageCreateInfo getCreateInfo() const;

  protected:
    ShaderModule m_module;

#ifdef DW_SHADER_ALLOW_NON_MAIN_ENTRY
    std::string m_entryPoint;

  public:
    IShader(ShaderModule&& mod, std::string const& entryPoint);
    NO_DISCARD const char* getEntryPoint() const { return m_entryPoint.c_str(); }

#else
  public:
    NO_DISCARD static constexpr const char* getEntryPoint() {
      return "main";
    }
#endif

    MOVE_CONSTRUCT_ONLY(IShader);
  };

  enum class ShaderStage {
    Vertex,
    Hull,
    Domain,
    Geometry,
    Fragment,
    Compute
  };

  template<ShaderStage TStage>
  class Shader : public IShader{
  public:
    const ShaderStage stage = TStage;

    Shader(ShaderModule&& mod);
    ~Shader() {}

    NO_DISCARD VkPipelineShaderStageCreateInfo getCreateInfo() const override;

    MOVE_CONSTRUCT_ONLY(Shader)
  };
}

#include "Shader.inl"

#endif
