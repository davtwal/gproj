// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : GraphicsPipelineCreator.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 25d
// * Last Altered: 2019y 09m 25d
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

#include "GraphicsPipeline.h"
#include "RenderPass.h"

#include <stdexcept>
#include <algorithm>

namespace dw {
  // Default pipeline state namespace
  namespace {
    VkPipelineVertexInputStateCreateInfo defaultPipelineVertexState = {
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      nullptr,
      0,
      0,
      nullptr,
      0,
      nullptr
    };

    VkPipelineInputAssemblyStateCreateInfo defaultPipelineAssemblyState = {
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      nullptr,
      0,
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      false
    };

    VkViewport                        defaultViewport              = {0, 0, 100, 100, 0, 1};
    VkRect2D                          defaultScissor               = {{0, 0}, {100, 100}};
    VkPipelineViewportStateCreateInfo defaultPipelineViewportState = {
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      nullptr,
      0,
      1,
      &defaultViewport,
      1,
      &defaultScissor
    };

    VkPipelineTessellationStateCreateInfo defaultPipelineTessellationState = {
      VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
      nullptr,
      0,
      0
    };

    VkPipelineRasterizationStateCreateInfo defaultPipelineRasterizationState = {
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      nullptr,
      0,
      false,
      false,
      VK_POLYGON_MODE_FILL,
      VK_CULL_MODE_BACK_BIT,
      VK_FRONT_FACE_CLOCKWISE,
      false,
      0.f,
      0.f,
      0.f,
      1.f
    };

    VkPipelineMultisampleStateCreateInfo defaultPipelineMultisampleState = {
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      nullptr,
      0,
      VK_SAMPLE_COUNT_1_BIT,
      false,
      1.f,
      nullptr,
      false,
      false
    };

    VkPipelineDepthStencilStateCreateInfo defaultPipelineDepthStencilState = {
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      nullptr,
      0,
      false,
      false,
      VK_COMPARE_OP_LESS,
      false,
      false,
      {
        VK_STENCIL_OP_ZERO,
        VK_STENCIL_OP_KEEP,
        VK_STENCIL_OP_ZERO,
        VK_COMPARE_OP_LESS,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF
      },
      {
        VK_STENCIL_OP_ZERO,
        VK_STENCIL_OP_KEEP,
        VK_STENCIL_OP_ZERO,
        VK_COMPARE_OP_LESS,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF
      },
      0.f,
      1.f
    };

    VkPipelineColorBlendStateCreateInfo defaultColorBlendState = {
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      nullptr,
      0,
      false,
      VK_LOGIC_OP_COPY,
      0,
      nullptr,
      {0, 0, 0, 0}
    };

    VkPipelineDynamicStateCreateInfo defaultDynamicState = {
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      nullptr,
      0,
      0,
      nullptr
    };
  }

  // Setup functions
  GraphicsPipelineCreator::GraphicsPipelineCreator()
    : vertexInput(defaultPipelineVertexState),
      inputAssembly(defaultPipelineAssemblyState),
      viewport(defaultPipelineViewportState),
      tessellation(defaultPipelineTessellationState),
      rasterization(defaultPipelineRasterizationState),
      multisample(defaultPipelineMultisampleState),
      depthstencil(defaultPipelineDepthStencilState),
      colorBlend(defaultColorBlendState),
      dynamic(defaultDynamicState) {

  }

  void GraphicsPipelineCreator::setVertexType(std::vector<VkVertexInputBindingDescription> const&   bindings,
                                              std::vector<VkVertexInputAttributeDescription> const& attribs) {
    vertexInput.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindings.size());
    vertexInput.pVertexBindingDescriptions      = bindings.data();
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribs.size());
    vertexInput.pVertexAttributeDescriptions    = attribs.data();
  }

  void GraphicsPipelineCreator::setAssemblyState(VkPrimitiveTopology topology, bool primitiveRestart) {
    inputAssembly.topology               = topology;
    inputAssembly.primitiveRestartEnable = (topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP
                                            || topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY
                                            || topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
                                            || topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
                                            || topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY)
                                             ? primitiveRestart
                                             : false;
  }

  VkViewport GraphicsPipelineCreator::fillViewport(VkExtent2D const& size,
                                                   float             x,
                                                   float             y,
                                                   float             minDepth,
                                                   float             maxDepth) {
    return VkViewport{x, y, static_cast<float>(size.width), static_cast<float>(size.height), minDepth, maxDepth};
  }

  GraphicsPipelineCreator& GraphicsPipelineCreator::setViewport(VkViewport const& viewport) {
    return *this;

  }

  GraphicsPipeline GraphicsPipelineCreator::finishCreate(LogicalDevice&   device,
                                                         VkPipelineLayout layout,
                                                         RenderPass       renderPass,
                                                         uint32_t         subPass,
                                                         bool             enableDepthStencil,
                                                         bool             enableMultisample,
                                                         bool             enableTessellation,
                                                         bool             enableRasterizer) {
    auto& ci = *this;

    // checks that everything is valid
    if (ci.shaders.empty())
      throw std::runtime_error("Attempted to create graphics pipeline with no shaders");

    if (!ci.vertexInput.pVertexBindingDescriptions) {
      if (std::find_if(ci.shaders.begin(),
                       ci.shaders.end(),
                       [](const VkPipelineShaderStageCreateInfo& s) {
                         return (s.flags & VK_SHADER_STAGE_MESH_BIT_NV);
                       }) == ci.shaders.end())
        throw std::runtime_error("No vertex binding/attributes declared, and no mesh shader introduced.");
    }

    if (enableTessellation && ci.tessellation.patchControlPoints == 0)
      throw std::runtime_error("Enabled tessellation but the number of patch control points is still 0");

    if (!enableRasterizer)
      ci.rasterization.rasterizerDiscardEnable = true;

    if (ci.colorBlend.attachmentCount == 0)
      throw std::runtime_error("No color blend attachments in pipeline");

    VkGraphicsPipelineCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(ci.shaders.size()),
      ci.shaders.data(),
      &ci.vertexInput,
      &ci.inputAssembly,
      enableTessellation ? &ci.tessellation : nullptr,
      enableRasterizer ? &ci.viewport : nullptr,
      &ci.rasterization,
      enableRasterizer && enableMultisample ? &ci.multisample : nullptr,
      enableDepthStencil ? &ci.depthstencil : nullptr,
      &ci.colorBlend,
      &ci.dynamic,
      layout,
      renderPass,
      subPass,
      nullptr,
      -1
    };

    GraphicsPipeline pipeline(device);

    // TODO: Pipeline cache, simultaneous graphics pipeline creates
    if (vkCreateGraphicsPipelines(device, nullptr, 0, &createInfo, nullptr, &pipeline.m_pipeline) != VK_SUCCESS || !
        pipeline.m_pipeline)
      throw std::runtime_error("Could not create pipeline");

    return pipeline;
  }
}
