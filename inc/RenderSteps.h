// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : RenderSteps.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 10m 10d
// * Last Altered: 2019y 11m 10d
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
    static constexpr unsigned MAX_MATERIALS               = 4;

    RenderStep(LogicalDevice& device);

    virtual ~RenderStep();

    //void setupRenderPass(std::vector<DependentImage> const& images);
    //void setupRenderPass(std::vector<Image> const& images);

    virtual void setupRenderPass(std::vector<util::Ref<Image>> const& images) = 0;
    virtual void setupShaders() = 0;
    virtual void setupPipeline(VkExtent2D extent) = 0;
    virtual void setupDescriptors() = 0;
    virtual void setupPipelineLayout(VkPipelineLayout layout = nullptr);

    // fb = output framebuffer from renderpass
    //virtual void writeCmdBuff(Framebuffer& fb, VkRect2D renderArea = {});
    NO_DISCARD RenderPass&       getRenderPass() const;
    NO_DISCARD GraphicsPipeline& getPipeline() const;
    NO_DISCARD VkPipelineLayout  getLayout() const;

  protected:
    static void renderScene(CommandBuffer&             commandBuff,
                            VkRenderPassBeginInfo&     beginInfo,
                            Scene::ObjContainer const& scene,
                            uint32_t                   alignment,
                            VkPipelineLayout           layout,
                            VkDescriptorSet            descriptorSet);

    friend class Renderer;
    VkPipelineLayout      m_layout{nullptr};
    VkDescriptorSetLayout m_descSetLayout{nullptr};
    VkDescriptorPool      m_descriptorPool{nullptr};

    // TODO: make this able to also be a compute pipeline
    util::ptr<GraphicsPipeline> m_pipeline;
    util::ptr<RenderPass>       m_pass;
  };


  class SplashScreenStep : public RenderStep {
    friend class Renderer;
  public:
    MOVE_CONSTRUCT_ONLY(SplashScreenStep);

    SplashScreenStep(LogicalDevice& device, CommandPool& pool, uint32_t imageCount);
    ~SplashScreenStep() override = default;

    void setupRenderPass(std::vector<util::Ref<Image>> const& images) override;
    void setupPipeline(VkExtent2D extent) override;
    void setupDescriptors() override;
    void setupShaders() override;

    // fb = output framebuffer from renderpass
    void writeCmdBuff(uint32_t index,
      Framebuffer const& fb,
      //Scene::ObjContainer const& scene,
      //uint32_t                   alignment,
      VkRect2D                   renderArea = {}) const;

    void updateDescriptorSets(
      uint32_t index,
      ImageView& logoView,
      VkSampler                sampler) const;

    NO_DISCARD CommandBuffer& getCommandBuffer(uint32_t nextImageIndex) const;

  private:
    std::vector<VkDescriptorSet>          m_descriptorSets;
    std::vector<util::Ref<CommandBuffer>> m_cmdBuffs;
    util::ptr<IShader>                    m_vertexShader;
    util::ptr<IShader>                    m_fragmentShader;

    uint32_t m_imageCount;
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

    GeometryStep(LogicalDevice& device, CommandPool& pool);
    ~GeometryStep() override = default;

    void setupRenderPass(std::vector<util::Ref<Image>> const& images) override;
    void setupPipeline(VkExtent2D extent) override;
    void setupDescriptors() override;
    void setupShaders() override;

    // fb = output framebuffer from renderpass
    void writeCmdBuff(Framebuffer&               fb,
                      Scene::ObjContainer const& scene,
                      uint32_t                   alignment,
                      VkRect2D                   renderArea = {}) const;

    void updateDescriptorSets(Buffer&                  modelUBO,
                              Buffer&                  cameraUBO,
                              Buffer&                  mtlUBO,
                              Buffer&                  shaderControlUBO,
                              MaterialManager::MtlMap& materialMap,
                              VkSampler                sampler) const;

    NO_DISCARD CommandBuffer& getCommandBuffer() const;

  private:
    util::ptr<IShader>       m_vertexShader;
    util::ptr<IShader>       m_fragmentShader;
    util::Ref<CommandBuffer> m_cmdBuff;
    VkDescriptorSet          m_descriptorSet{nullptr};
  };

  class ShadowMapStep : public RenderStep {
    friend class Renderer;
  public:
  MOVE_CONSTRUCT_ONLY(ShadowMapStep);

    ShadowMapStep(LogicalDevice& device, CommandPool& pool);
    ~ShadowMapStep() override = default;

    void setupRenderPass(std::vector<util::Ref<Image>> const& images) override;
    void setupPipeline(VkExtent2D extent) override;
    void setupDescriptors() override;
    void setupPipelineLayout(VkPipelineLayout layout = nullptr) override;
    void setupShaders() override;

    void writeCmdBuff(std::vector<Renderer::ShadowMappedLight> const& lights,
                      Scene::ObjContainer const&                      scene,
                      uint32_t                                        alignment,
                      VkRect2D                                        renderArea = {}) const;

    void updateDescriptorSets(Buffer& modelUBO, Buffer& lightsUBO) const;

    NO_DISCARD CommandBuffer& getCommandBuffer() const;

  private:
    util::ptr<IShader>       m_vertexShader;
    util::ptr<IShader>       m_fragmentShader;
    util::Ref<CommandBuffer> m_cmdBuff;
    VkDescriptorSet          m_descriptorSet{nullptr};
  };

  class BlurStep : public RenderStep {
    friend class Renderer;
  public:
  MOVE_CONSTRUCT_ONLY(BlurStep);

    BlurStep(LogicalDevice& device, CommandPool& pool);
    ~BlurStep() override;

    // ironically there are no render passes here and this function is useless
    void setupRenderPass(std::vector<util::Ref<Image>> const& images) override;
    void setupPipelineLayout(VkPipelineLayout layout = nullptr) override;
    void setupPipeline(VkExtent2D extent) override;
    void setupDescriptors() override;
    void setupShaders() override;

    void writeCmdBuff(std::vector<Renderer::ShadowMappedLight> const& lights,
                      DependentImage&                                 intermediaryImg,
                      ImageView&                                      intermediaryView);

    NO_DISCARD CommandBuffer& getCommandBuffer() const;

  private:
    struct DescriptorSetCont {
      VkDescriptorSet x{nullptr};
      VkDescriptorSet y{nullptr};
    };

    void updateDescriptorSets(DescriptorSetCont const& set,
                              ImageView&               source,
                              ImageView&               intermediate,
                              ImageView&               dest,
                              VkSampler                sampler = nullptr) const;
    util::ptr<IShader>       m_blur_x;
    util::ptr<IShader>       m_blur_y;
    util::Ref<CommandBuffer> m_cmdBuff;
    VkPipeline               m_compute_x{nullptr};
    VkPipeline               m_compute_y{nullptr};

    std::vector<DescriptorSetCont> m_descriptorSets;

    //VkDescriptorSet          m_descriptorSet_x{ nullptr };
    //VkDescriptorSet          m_descriptorSet_y{ nullptr };
  };

  class GlobalLightStep : public RenderStep {
    friend class Renderer;
  public:
    static constexpr uint32_t MAX_GLOBAL_LIGHTS = 2;
    static constexpr uint32_t MAX_IMPORTANCE_SAMPLES = 32;

    struct ImportanceSampleUBO {
      float samples[2 * MAX_IMPORTANCE_SAMPLES];
      //alignas(04) glm::vec3 padding{ 0 };
    };

  MOVE_CONSTRUCT_ONLY(GlobalLightStep);

    GlobalLightStep(LogicalDevice& device, CommandPool& pool);
    ~GlobalLightStep() override = default;

    void setupRenderPass(std::vector<util::Ref<Image>> const& images) override;
    void setupPipelineLayout(VkPipelineLayout layout = nullptr) override;
    void setupPipeline(VkExtent2D extent) override;
    void setupDescriptors() override;
    void setupShaders() override;

    void writeCmdBuff(Framebuffer& fb, VkRect2D renderArea = {}) const;

    void updateDescriptorSets(std::vector<ImageView> const&                   gbufferViews,
                              std::vector<Renderer::ShadowMappedLight> const& lights,
                              ImageView&                                      backgroundImg,
                              ImageView&                                      irradianceImg,
                              Buffer&                                         cameraUBO,
                              Buffer&                                         lightsUBO,
                              Buffer&                                         importanceSamplesUBO,
                              Buffer&                                         shaderControlUBO,
                              VkSampler                                       sampler) const;

    NO_DISCARD CommandBuffer& getCommandBuffer() const;

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

    static constexpr uint32_t MAX_LOCAL_LIGHTS = 128;

    FinalStep(LogicalDevice& device, CommandPool& pool, uint32_t numSwapchainImages);
    ~FinalStep() override = default;

    void setupRenderPass(std::vector<util::Ref<Image>> const& images) override;
    void setupPipeline(VkExtent2D extent) override;
    void setupDescriptors() override;
    void setupShaders() override;

    void writeCmdBuff(std::vector<Framebuffer> const& fbs,
                      Image const&                    previousImage,
                      VkRect2D                        renderArea = {},
                      uint32_t                        image      = ~0u);
    void updateDescriptorSets(std::vector<ImageView> const& gbufferViews,
                              ImageView const&              previousImage,
                              Buffer&                       cameraUBO,
                              Buffer&                       lightsUBO,
                              Buffer&                       shaderControlUBO,
                              VkSampler                     sampler);

    NO_DISCARD CommandBuffer& getCommandBuffer(uint32_t index);
  private:
    std::vector<VkDescriptorSet>          m_descriptorSets;
    std::vector<util::Ref<CommandBuffer>> m_cmdBuffs;
    util::ptr<IShader>                    m_vertexShader;
    util::ptr<IShader>                    m_fragmentShader;

    uint32_t m_imageCount;
  };
}

#endif
