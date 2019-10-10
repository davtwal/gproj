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
#include <cassert>

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
      VK_FRONT_FACE_COUNTER_CLOCKWISE,
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
    : m_vertexInput(defaultPipelineVertexState),
      m_inputAssembly(defaultPipelineAssemblyState),
      m_viewport(defaultPipelineViewportState),
      m_tessellation(defaultPipelineTessellationState),
      m_rasterization(defaultPipelineRasterizationState),
      m_multisample(defaultPipelineMultisampleState),
      m_depthstencil(defaultPipelineDepthStencilState),
      m_colorBlend(defaultColorBlendState),
      m_dynamic(defaultDynamicState) {

  }

  // VERTEX INPUT
  void GraphicsPipelineCreator::setVertexType(std::vector<VkVertexInputBindingDescription> const&   bindings,
                                              std::vector<VkVertexInputAttributeDescription> const& attribs) {
    m_bindings = bindings;
    m_attribs = attribs;
    m_vertexInput.vertexBindingDescriptionCount   = static_cast<uint32_t>(m_bindings.size());
    m_vertexInput.pVertexBindingDescriptions      = m_bindings.data();
    m_vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_attribs.size());
    m_vertexInput.pVertexAttributeDescriptions    = m_attribs.data();
  }

  // INPUT ASSEMBLY
  void GraphicsPipelineCreator::setAssemblyState(VkPrimitiveTopology topology, bool primitiveRestart) {
    m_inputAssembly.topology               = topology;
    m_inputAssembly.primitiveRestartEnable = (topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP
                                            || topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY
                                            || topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
                                            || topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
                                            || topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY)
                                             ? primitiveRestart
                                             : false;
  }

  // VIEWPORTS
  
  VkViewport GraphicsPipelineCreator::fillViewport(VkExtent2D const& size,
                                                   float             x,
                                                   float             y,
                                                   float             minDepth,
                                                   float             maxDepth) {
    return VkViewport{x, y, static_cast<float>(size.width), static_cast<float>(size.height), minDepth, maxDepth};
  }

  GraphicsPipelineCreator& GraphicsPipelineCreator::setViewport(VkViewport const& viewport) {
    return setViewports({viewport});
  }

  GraphicsPipelineCreator& GraphicsPipelineCreator::setViewports(std::vector<VkViewport> const& viewports) {
    m_viewports = viewports;
    m_viewport.viewportCount = static_cast<uint32_t>(m_viewports.size());
    m_viewport.pViewports = m_viewports.data();
    return *this;
  }

  GraphicsPipelineCreator& GraphicsPipelineCreator::setScissor(VkRect2D const& scissor) {
    return setScissors({ scissor });
  }

  GraphicsPipelineCreator& GraphicsPipelineCreator::setScissors(std::vector<VkRect2D> const& scissors) {
    m_scissors = scissors;
    m_viewport.scissorCount = static_cast<uint32_t>(m_scissors.size());
    m_viewport.pScissors = m_scissors.data();
    return *this;
  }

  // RASTERIZER
  GraphicsPipelineCreator& GraphicsPipelineCreator::setPolygonMode(VkPolygonMode mode) {
    m_rasterization.polygonMode = mode;
    return *this;
  }

  GraphicsPipelineCreator& GraphicsPipelineCreator::setCullMode(VkCullModeFlags mode) {
    m_rasterization.cullMode = mode;
    return *this;
  }

  GraphicsPipelineCreator& GraphicsPipelineCreator::setFrontFace(bool clockwise) {
    m_rasterization.frontFace = clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
    return *this;
  }

  GraphicsPipelineCreator& GraphicsPipelineCreator::setLineWidth(float w) {
    m_rasterization.lineWidth = w;
    return *this;
  }

  GraphicsPipelineCreator& GraphicsPipelineCreator::setDepthInfo(bool depthClamp, bool depthBias, float biasConst, float biasClamp, float biasSlope) {
    m_rasterization.depthClampEnable = depthClamp;
    m_rasterization.depthBiasEnable = depthBias;
    m_rasterization.depthBiasConstantFactor = biasConst;
    m_rasterization.depthBiasClamp = biasClamp;
    m_rasterization.depthBiasSlopeFactor = biasSlope;
    return *this;
  }

  // MULTISAMPLE

  GraphicsPipelineCreator& GraphicsPipelineCreator::setSampleCount(VkSampleCountFlagBits flags) {
    m_multisample.rasterizationSamples = flags;
    return *this;
  }

  GraphicsPipelineCreator& GraphicsPipelineCreator::setSampleCount(uint32_t count) {
    assert(count < 0x7F);
    return setSampleCount(count);
  }

  GraphicsPipelineCreator& GraphicsPipelineCreator::setSampleShading(bool enable, float minSampleShading) {
    m_multisample.sampleShadingEnable = enable;
    m_multisample.minSampleShading = minSampleShading;
    return *this;
  }

  GraphicsPipelineCreator& GraphicsPipelineCreator::setSampleAlpha(bool toOne, bool toCoverage) {
    m_multisample.alphaToOneEnable = toOne;
    m_multisample.alphaToCoverageEnable = toCoverage;
    return *this;
  }

  // DEPTH & STENCIL

  GraphicsPipelineCreator& GraphicsPipelineCreator::setDepthTesting(bool enabled, VkCompareOp compareOp, bool boundsTest, bool depthWrite) {
    m_depthstencil.depthTestEnable = enabled;
    m_depthstencil.depthWriteEnable = depthWrite;
    m_depthstencil.depthCompareOp = compareOp;
    m_depthstencil.depthBoundsTestEnable = boundsTest;
    return *this;
  }

  // COLOR BLEND

  GraphicsPipelineCreator& GraphicsPipelineCreator::addAttachment(VkPipelineColorBlendAttachmentState const& attachment) {
    m_colorBlendStates.push_back(attachment);
    m_colorBlend.attachmentCount = static_cast<uint32_t>(m_colorBlendStates.size());
    m_colorBlend.pAttachments = m_colorBlendStates.data();
    return *this;
  }

  GraphicsPipelineCreator& GraphicsPipelineCreator::setAttachments(std::vector<VkPipelineColorBlendAttachmentState> const& attachments) {
    m_colorBlendStates = attachments;
    m_colorBlend.attachmentCount = static_cast<uint32_t>(m_colorBlendStates.size());
    m_colorBlend.pAttachments = m_colorBlendStates.data();
    return *this;
  }

  // SHADERS

  GraphicsPipelineCreator& GraphicsPipelineCreator::addShaderStage(VkPipelineShaderStageCreateInfo const& info) {
    m_shaders.push_back(info);
    return *this;
  }

  GraphicsPipelineCreator& GraphicsPipelineCreator::setShaderStages(std::vector<VkPipelineShaderStageCreateInfo> const& infos) {
    m_shaders = infos;
    return *this;
  }
  
  // FINISH

  GraphicsPipeline GraphicsPipelineCreator::finishCreate(LogicalDevice&   device,
                                                         VkPipelineLayout layout,
                                                         RenderPass&      renderPass,
                                                         uint32_t         subPass,
                                                         bool             enableDepthStencil,
                                                         bool             enableTessellation,
                                                         bool             enableRasterizer) {
    // checks that everything is valid
    if (m_shaders.empty())
      throw std::runtime_error("Attempted to create graphics pipeline with no shaders");

    /*if (!m_vertexInput.pVertexBindingDescriptions) {
      if (std::find_if(m_shaders.begin(),
        m_shaders.end(),
                       [](const VkPipelineShaderStageCreateInfo& s) {
                         return (s.flags & VK_SHADER_STAGE_MESH_BIT_NV);
                       }) == m_shaders.end())
        throw std::runtime_error("No vertex binding/attributes declared, and no mesh shader introduced.");
    }*/

    if (enableTessellation && m_tessellation.patchControlPoints == 0)
      throw std::runtime_error("Enabled tessellation but the number of patch control points is still 0");

    if (!enableRasterizer)
      m_rasterization.rasterizerDiscardEnable = true;

    if (m_colorBlend.attachmentCount == 0)
      throw std::runtime_error("No color blend attachments in pipeline");

    VkGraphicsPipelineCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(m_shaders.size()),
      m_shaders.data(),
      &m_vertexInput,
      &m_inputAssembly,
      enableTessellation ? &m_tessellation : nullptr,
      enableRasterizer ? &m_viewport : nullptr,
      &m_rasterization,
      enableRasterizer ? &m_multisample : nullptr,
      enableDepthStencil ? &m_depthstencil : nullptr,
      &m_colorBlend,
      &m_dynamic,
      layout,
      renderPass,
      subPass,
      nullptr,
      -1
    };

    GraphicsPipeline pipeline(device);

    // TODO: Pipeline cache, simultaneous graphics pipeline creates
    if (vkCreateGraphicsPipelines(device, nullptr, 1, &createInfo, nullptr, &pipeline.m_pipeline) != VK_SUCCESS || !
        pipeline.m_pipeline)
      throw std::runtime_error("Could not create pipeline");

    return pipeline;
  }
}
