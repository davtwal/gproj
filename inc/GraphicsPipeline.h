// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : GraphicsPipeline.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 24d
// * Last Altered: 2019y 09m 24d
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

#ifndef DW_GRAPHICS_PIPELINE_H
#define DW_GRAPHICS_PIPELINE_H

#include "LogicalDevice.h"

namespace dw {
  class RenderPass;

  CREATE_DEVICE_DEPENDENT(GraphicsPipeline)
  public:
    GraphicsPipeline(LogicalDevice& device);
    ~GraphicsPipeline();

    operator VkPipeline() const;

  private:
    friend class GraphicsPipelineCreator;
    VkPipeline m_pipeline{nullptr};
  };

  class GraphicsPipelineCreator {
  public:
    GraphicsPipelineCreator();
    VkPipelineVertexInputStateCreateInfo         m_vertexInput;
    VkPipelineInputAssemblyStateCreateInfo       m_inputAssembly;
    VkPipelineViewportStateCreateInfo            m_viewport;
    std::vector<VkViewport> m_viewports;
    std::vector<VkRect2D> m_scissors;
    VkPipelineTessellationStateCreateInfo        m_tessellation;
    VkPipelineRasterizationStateCreateInfo       m_rasterization;
    VkPipelineMultisampleStateCreateInfo         m_multisample;
    VkPipelineDepthStencilStateCreateInfo        m_depthstencil;
    VkPipelineColorBlendStateCreateInfo          m_colorBlend;
    VkPipelineDynamicStateCreateInfo             m_dynamic;
    std::vector<VkVertexInputBindingDescription> m_bindings;
    std::vector<VkVertexInputAttributeDescription> m_attribs;
    std::vector<VkPipelineShaderStageCreateInfo> m_shaders;
    std::vector<VkPipelineColorBlendAttachmentState> m_colorBlendStates;

    /* What can be dynamic in a graphics pipeline:
     *  - Viewport              ViewportState
     *  - Scissor               ViewportState
     *  - Line width            RasterizerState
     *  - Depth bias            RasterizerState
     *  - Depth constants       RasterizerState
     *  - Depth bounds          RasterizerState
     *  - Stencil compare mask  DepthStencilState
     *  - Stencil write mask    DepthStencilState
     *  - Stencil reference (?) DepthStencilState
     *
     * Dynamic with extensions:
     *  - Viewport w/ Scaling (NV)
     *  - Discard rectangle (EXT)
     *  - Sample locations (EXT)
     *  - Viewport shading rate palette (NV) (?)
     *  - Viewport coarse sample order (NV) (?)
     *  - Exclusive scissor
     */

    /* The different parts of the pipeline we can adjust:
     *  - Vertex Input**    (Binding/attribute descriptions)
     *  - Input assembly**  (Primitive type, restart enable)
     *  - Tesselation*      ()
     *  - Viewport state+   (Viewport[s] & scissor[s])
     *  - Rasterizer state  (Polygon type, front face & culling, depth adjustment)
     *  - Multi-sampling+   (Count, sample shading, mask...)
     *  - Depth stencil+=   ()
     *  - Color blend+/     ()
     *
     * STAGES
     *  - Vertex            -> makes vertex input & input assembly required
     *  - Tess. Control     -> optional unless VK_PRIMITIVE_TOPOLOGY_PATCH_LIST
     *  - Tess. Evaluation  -> optional unless VK_PRIMITIVE_TOPOLOGY_PATCH_LIST
     *  - Geometry          -> optional
     *  - Fragment
     *  / All Graphics
     *  / All
     *  x Ray-gen       RT
     *  x Any hit       RT
     *  x Closest hit   RT
     *  x Miss          RT
     *  x Intersection  RT
     *  x Callable      RT
     *  x Task          MSH -> disables vertex input & assembly stages
     *  x Mesh          MSH -> disables vertex input & assembly stages
     *
     * *  = ignored if no tesselation control AND no tess eval
     * ** = ignored if there is a mesh shader stage (NV)
     * +  = ignored if pipeline has rasterization disabled (e.g rasterizerDiscardEnable == true)
     * =  = ignored if the subpass of the renderpass does not have a depth-stencil attachment
     * /  = ignored if the subpass of the renderpass does not have any color attachments
     */


    // SETUP PIPELINE
    // 
    //GraphicsPipelineCreator& reset();

    // VERTEX INPUT
    void setVertexType(std::vector<VkVertexInputBindingDescription> const&   bindings,
                       std::vector<VkVertexInputAttributeDescription> const& attribs);
  
    // INPUT ASSEMBLY
    void setAssemblyState(VkPrimitiveTopology topology         = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                          bool                primitiveRestart = false);
  
    // TESSELLATION

    // VIEWPORT
    static VkViewport fillViewport(VkExtent2D const& size,
                                   float             x        = 0.f,
                                   float             y        = 0.f,
                                   float             minDepth = 0.f,
                                   float             maxDepth = 1.f);
    GraphicsPipelineCreator& setViewport(VkViewport const& viewport);                 // Default: {0,0}, {100,100}, 0, 1
    GraphicsPipelineCreator& setViewports(std::vector<VkViewport> const& viewports);  // Must be the same number as the number of scissors
    GraphicsPipelineCreator& setScissor(VkRect2D const& scissor);                 // Default: {0, 0}, {100, 100}
    GraphicsPipelineCreator& setScissors(std::vector<VkRect2D> const& scissors);  // Must be the same number as the number of viewports

    // RASTERIZER
    GraphicsPipelineCreator& setPolygonMode(VkPolygonMode mode);  // Default: FILL
    GraphicsPipelineCreator& setCullMode(VkCullModeFlags mode);   // Default: BACK
    GraphicsPipelineCreator& setFrontFace(bool clockwise);        // Default: CLOCKWISE
    GraphicsPipelineCreator& setLineWidth(float w = 1.f);
    GraphicsPipelineCreator& setDepthInfo(bool  depthClamp = false,
                                          bool  depthBias  = false,
                                          float biasConst  = 0.f,
                                          float biasClamp  = 0.f,
                                          float biasSlope  = 0.f);
  
    // MULTISAMPLE
    GraphicsPipelineCreator& setSampleCount(VkSampleCountFlagBits flags);
    GraphicsPipelineCreator& setSampleCount(uint32_t count = VK_SAMPLE_COUNT_1_BIT);
    GraphicsPipelineCreator& setSampleShading(bool enable = false, float minSampleShading = 1.f);
    GraphicsPipelineCreator& setSampleAlpha(bool toOne = false, bool toCoverage = false);
  
    // DEPTH STENCIL
    GraphicsPipelineCreator& setDepthTesting(bool        enabled,
                                             VkCompareOp compareOp  = VK_COMPARE_OP_LESS,
                                             bool        boundsTest = false,
                                             bool        depthWrite = true);
    // TODO: other depth stencil stuff

    // COLOR BLEND
    GraphicsPipelineCreator& addAttachment(VkPipelineColorBlendAttachmentState const& attachment);
    GraphicsPipelineCreator& setAttachments(std::vector<VkPipelineColorBlendAttachmentState> const& attachments);;

    // DYNAMIC
    // TODO

    // SHADER STAGES
    GraphicsPipelineCreator& addShaderStage(VkPipelineShaderStageCreateInfo const& info);
    GraphicsPipelineCreator& setShaderStages(std::vector<VkPipelineShaderStageCreateInfo> const& infos);

    // FINISH
    NO_DISCARD GraphicsPipeline finishCreate(LogicalDevice&   device,
                                             VkPipelineLayout layout,
                                             RenderPass&      renderPass,
                                             uint32_t         subPass,
                                             bool             enableDepthStencil = false,
                                             bool             enableTessellation = false,
                                             bool             enableRasterizer   = true);
  };
}

#endif
