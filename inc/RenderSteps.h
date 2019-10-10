// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : RenderSteps.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 10m 09d
// * Last Altered: 2019y 10m 09d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#ifndef DW_RENDER_STEP_H
#define DW_RENDER_STEP_H

#include "Renderer.h"

#include "GraphicsPipeline.h"
#include "LogicalDevice.h"
#include "Image.h"
#include "CommandBuffer.h"
#include "Framebuffer.h"
#include "Shader.h"

namespace dw {
  /* Data that is independent of the render steps, and must be fed in:
   *  - Instance, device
   *  - Surface, 
   *  - Frame buffers (e.g. deferred buffer)
   *  - Uniform buffers, these should be updated outside
   *  - Command pool
   *  - Queues
   *  - Semaphores
   *  - Fences (?)
   *
   * Data that is contained in the render steps:
   *  - Pipelines & layout
   *  - Descriptors & layout
   *  - Render pass
   *  - Command buffer
   *  - Shaders
   */

  CREATE_DEVICE_DEPENDENT(RenderStep)
  public:
    static constexpr unsigned NUM_EXPECTED_GBUFFER_IMAGES = 3;

    RenderStep(LogicalDevice& device);

    virtual ~RenderStep();

    //void setupRenderPass(std::vector<DependentImage> const& images);
    //void setupRenderPass(std::vector<Image> const& images);

    virtual void setupRenderPass(std::vector<util::Ref<Image>> const& images) = 0;
    virtual void setupShaders() = 0;
    virtual void setupPipeline(VkExtent2D extent) = 0;
    virtual void setupDescriptors() = 0;
    virtual void setupPipelineLayout() = 0;

    // fb = output framebuffer from renderpass
    //virtual void writeCmdBuff(Framebuffer& fb, VkRect2D renderArea = {});
    RenderPass& getRenderPass();
    GraphicsPipeline& getPipeline();

  protected:
    friend class Renderer;
    VkPipelineLayout      m_layout{ nullptr };
    VkDescriptorSetLayout m_descSetLayout{ nullptr };
    VkDescriptorPool      m_descriptorPool{ nullptr };

    util::ptr<GraphicsPipeline> m_pipeline;
    util::ptr<RenderPass>       m_pass;
  };

  /* Geometry Pass:
   *  - Output framebuffer:
   *    + 3x R32G32B32A32 SFLOAT
   *    + 1x D24_S8
   */

  class GeometryStep : public RenderStep {
    friend class Renderer;
  public:
    MOVE_CONSTRUCT_ONLY(GeometryStep);
    using RenderStep::setupRenderPass;

    GeometryStep(LogicalDevice& device, CommandPool& pool);
    ~GeometryStep() override = default;

    void setupRenderPass(std::vector<util::Ref<Image>> const& images) override;
    void setupPipeline(VkExtent2D extent) override;
    void setupDescriptors() override;
    void setupShaders() override;
    void setupPipelineLayout() override;

    // fb = output framebuffer from renderpass
    void writeCmdBuff(Framebuffer&                    fb,
                      Renderer::SceneContainer const& scene,
                      uint32_t                        alignment,
                      VkRect2D                        renderArea = {}) const;

    void updateDescriptorSets(Buffer& modelUBO, Buffer& cameraUBO) const;

    NO_DISCARD CommandBuffer& getCommandBuffer();

  private:
    util::ptr<IShader>       m_vertexShader;
    util::ptr<IShader>       m_fragmentShader;
    util::Ref<CommandBuffer> m_cmdBuff;
    VkDescriptorSet          m_descriptorSet{nullptr};
  };

  class FinalStep : public RenderStep {
    friend class Renderer;
  public:
    MOVE_CONSTRUCT_ONLY(FinalStep);

    FinalStep(LogicalDevice& device, CommandPool& pool, uint32_t numSwapchainImages);
    ~FinalStep() override = default;

    void setupRenderPass(std::vector<util::Ref<Image>> const& images) override;
    void setupPipeline(VkExtent2D extent) override;
    void setupDescriptors() override;
    void setupShaders() override;
    void setupPipelineLayout() override;

    void writeCmdBuff(std::vector<Framebuffer> const& fbs, VkRect2D renderArea = {});
    void updateDescriptorSets(std::vector<ImageView> const& gbufferViews,
                              Buffer&                       cameraUBO,
                              Buffer&                       lightsUBO,
                              VkSampler                     sampler);

    NO_DISCARD CommandBuffer& getCommandBuffer(uint32_t index);
  private:
    std::vector<VkDescriptorSet>          m_descriptorSets;
    std::vector<util::Ref<CommandBuffer>> m_cmdBuffs;
    util::ptr<IShader>                    m_vertexShader;
    util::ptr<IShader>                    m_fragmentShader;

    uint32_t                              m_imageCount;
  };
}

#endif