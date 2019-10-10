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
    RenderStep(LogicalDevice& device);

    virtual ~RenderStep();

    virtual void setup(Renderer& r);
    virtual void allocateCommandBuffer(CommandPool& pool);

    virtual void setupRenderPass();
    virtual void setupShaders();
    virtual void setupPipeline(VkExtent2D extent);
    virtual void setupDescriptors();
    virtual void setupPipelineLayout();

    // fb = output framebuffer from renderpass
    //virtual void writeCmdBuff(Framebuffer& fb, VkRect2D renderArea = {});

    virtual void submit(Queue& q);

  protected:
    friend class Renderer;
    VkPipelineLayout m_layout;
    VkDescriptorSetLayout m_descSetLayout;
    VkDescriptorPool m_descriptorPool;

    util::ptr<GraphicsPipeline> m_pipeline;
    util::ptr<RenderPass> m_pass;
    util::Ref<CommandBuffer> m_cmdBuff;
  };

  /* Geometry Pass:
   *  - Output framebuffer:
   *    + 3x R32G32B32A32 SFLOAT
   *    + 1x D24_S8
   */

  class GeometryStep : public RenderStep {
    friend class Renderer;
  public:
    GeometryStep(LogicalDevice& device);

    void setup(Renderer& r) override;
    void setupRenderPass() override;
    void setupPipeline(VkExtent2D extent) override;
    void setupDescriptors() override;
    void setupShaders() override;
    void setupPipelineLayout() override;

    // fb = output framebuffer from renderpass
    void writeCmdBuff(Framebuffer& fb, Renderer::SceneContainer const& scene, uint32_t alignment, VkRect2D renderArea = {});

    void updateDescriptorSets(Buffer& modelUBO, Buffer& cameraUBO);

    void submit(Queue& q);

  private:
    util::ptr<IShader> m_vertexShader;
    util::ptr<IShader> m_fragmentShader;
  };

  class FinalStep : public RenderStep {
    friend class Renderer;
  public:
    FinalStep(LogicalDevice& device, uint32_t numSwapchainImages);

    void setup(Renderer& r) override;
    void setupRenderPass() override;
    void setupPipeline(VkExtent2D extent) override;
    void setupDescriptors() override;
    void setupShaders() override;
    void setupPipelineLayout() override;

  private:
    std::vector<VkDescriptorSet> m_descriptorSets;
    util::ptr<IShader> m_vertexShader;
    util::ptr<IShader> m_fragmentShader;
    uint32_t m_imageCount;
  };
}

#endif
