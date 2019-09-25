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

    // SETUP PIPELINE
    // 

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
    GraphicsPipeline& setViewport(VkViewport const& viewport);                 // Default: {0,0}, {100,100}, 0, 1
    GraphicsPipeline& setViewports(std::vector<VkViewport> const& viewports);  // Must be the same number as the number of scissors
    GraphicsPipeline& setScissor(VkRect2D const& scissor);                 // Default: {0, 0}, {100, 100}
    GraphicsPipeline& setScissors(std::vector<VkRect2D> const& scissors);  // Must be the same number as the number of viewports

    // RASTERIZER
    GraphicsPipeline& setPolygonMode(VkPolygonMode mode);  // Default: FILL
    GraphicsPipeline& setCullMode(VkCullModeFlags mode);   // Default: BACK
    GraphicsPipeline& setFrontFace(bool clockwise);        // Default: CLOCKWISE
    GraphicsPipeline& setLineWidth(float w = 1.f);
    GraphicsPipeline& setDepthInfo(bool  depthClamp = false,
                                   bool  depthBias  = false,
                                   float biasConst  = 0.f,
                                   float biasClamp  = 0.f,
                                   float biasSlope  = 0.f);

    // MULTISAMPLE
    GraphicsPipeline& setSampleCount(VkSampleCountFlagBits flags);
    GraphicsPipeline& setSampleCount(uint32_t count = VK_SAMPLE_COUNT_1_BIT);
    GraphicsPipeline& setSampleShading(bool enable = false, float minSampleShading = 1.f);
    GraphicsPipeline& setSampleAlpha(bool toOne = false, bool toCoverage = false);

    // DEPTH STENCIL

    // COLOR BLEND
    GraphicsPipeline& setBlendFactor(VkBlendFactor source      = VK_BLEND_FACTOR_SRC_ALPHA,
                        VkBlendFactor dest        = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                        VkBlendFactor sourceAlpha = VK_BLEND_FACTOR_ONE,
                        VkBlendFactor destAlpha   = VK_BLEND_FACTOR_ZERO);

    GraphicsPipeline& setBlendOps(VkBlendOp color = VK_BLEND_OP_ADD, VkBlendOp alpha = VK_BLEND_OP_ADD);
    GraphicsPipeline& setColorWriteMask(VkColorComponentFlags flags = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
    GraphicsPipeline& addAttachment();
    GraphicsPipeline& setBlendConstants(float constants[4]);
    GraphicsPipeline& setLogicOp(bool enable, VkLogicOp op = VK_LOGIC_OP_COPY);

    // DYNAMIC

    // FINISH
    void finishCreate(VkPipelineLayout layout,
                      RenderPass       renderPass,
                      uint32_t         subPass,
                      bool             enableDepthStencil = false,
                      bool             enableMultisample  = false,
                      bool             enableTessellation = false,
                      bool             enableRasterizer   = true);

    //NO_DISCARD GraphicsPipeline createDerivative() const;

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


  private:
    VkPipeline m_pipeline;

    struct PipelineCreateInfo {
      PipelineCreateInfo();
      VkPipelineVertexInputStateCreateInfo   vertexInput;
      VkPipelineInputAssemblyStateCreateInfo inputAssembly;
      VkPipelineViewportStateCreateInfo      viewport;
      VkPipelineTessellationStateCreateInfo  tessellation;
      VkPipelineRasterizationStateCreateInfo rasterization;
      VkPipelineMultisampleStateCreateInfo   multisample;
      VkPipelineDepthStencilStateCreateInfo  depthstencil;
      VkPipelineColorBlendStateCreateInfo    colorBlend;
      VkPipelineDynamicStateCreateInfo       dynamic;
    };

    PipelineCreateInfo* m_currCreateInfo{nullptr};
  };
}

#endif
