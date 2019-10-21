// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Render_GlobalLighting.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 10m 15d
// * Last Altered: 2019y 10m 15d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "Renderer.h"
#include "RenderSteps.h"
#include "Framebuffer.h"
#include "CommandBuffer.h"
#include "Shader.h"
#include "Image.h"

#include <stdexcept>
#include <array>

namespace dw {
  GlobalLightStep::GlobalLightStep(LogicalDevice& device, CommandPool& pool)
    : RenderStep(device),
      m_cmdBuff(pool.allocateCommandBuffer()) {
  }

  GlobalLightStep::GlobalLightStep(GlobalLightStep&& o) noexcept
    : RenderStep(std::move(o)),
      m_vertexShader(std::move(o.m_vertexShader)),
      m_fragmentShader(std::move(o.m_fragmentShader)),
      m_cmdBuff(o.m_cmdBuff),
      m_descriptorSet(o.m_descriptorSet) {
    o.m_vertexShader   = nullptr;
    o.m_fragmentShader = nullptr;
    o.m_descriptorSet  = nullptr;
  }

  CommandBuffer& GlobalLightStep::getCommandBuffer() const {
    return m_cmdBuff;
  }

  void GlobalLightStep::setupShaders() {
    m_vertexShader = util::make_ptr<Shader<ShaderStage::Vertex>>(
      ShaderModule::Load(getOwningDevice(),
      "fsq_vert.spv"
    ));
    m_fragmentShader = util::make_ptr<Shader<ShaderStage::Fragment>>(
      ShaderModule::Load(getOwningDevice(),
      "global_lighting_frag.spv"
    ));
  }

  void GlobalLightStep::setupDescriptors() {
    std::vector<VkDescriptorSetLayoutBinding> finalBindings;
    finalBindings.
        resize(NUM_EXPECTED_GBUFFER_IMAGES + 2 + 1); // one sampler per gbuffer image + one camera + lighting ubo + 

    finalBindings[0] = {
      0,
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      1,
      VK_SHADER_STAGE_FRAGMENT_BIT,
      nullptr
    };

    finalBindings[1] = {
      1,
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      1,
      VK_SHADER_STAGE_FRAGMENT_BIT,
      nullptr
    };

    for (uint32_t i = 2; i < NUM_EXPECTED_GBUFFER_IMAGES + 2 + 1; ++i) {
      finalBindings[i] = {
        i,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        1,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        nullptr
      };
    }

    finalBindings.back().descriptorCount = MAX_GLOBAL_LIGHTS;

    VkDescriptorSetLayoutCreateInfo finalLayoutCreate = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(finalBindings.size()),
      finalBindings.data()
    };

    if (vkCreateDescriptorSetLayout(getOwningDevice(), &finalLayoutCreate, nullptr, &m_descSetLayout) != VK_SUCCESS)
      throw std::runtime_error("could not create final descriptor set");

    // pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
      {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        2
      },
      {
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        NUM_EXPECTED_GBUFFER_IMAGES + MAX_GLOBAL_LIGHTS
      }
    };

    VkDescriptorPoolCreateInfo finalPoolInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      nullptr,
      0,
      1,
      static_cast<uint32_t>(poolSizes.size()),
      poolSizes.data()
    };

    if (vkCreateDescriptorPool(getOwningDevice(), &finalPoolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
      throw std::runtime_error("could not create global lighting descriptor pool");

    VkDescriptorSetAllocateInfo allocateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      nullptr,
      m_descriptorPool,
      1,
      &m_descSetLayout
    };

    if (vkAllocateDescriptorSets(getOwningDevice(), &allocateInfo, &m_descriptorSet) != VK_SUCCESS)
      throw std::runtime_error("could not create global lighting descriptor sets");

  }

  void GlobalLightStep::setupRenderPass(std::vector<util::Ref<Image>> const& images) {
    assert(images.size() == 1);

    m_pass = util::make_ptr<RenderPass>(getOwningDevice());
    m_pass->addAttachment(images[0].get().getAttachmentDesc(VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                            VK_ATTACHMENT_STORE_OP_STORE,
                                                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
    m_pass->addAttachmentRef(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, RenderPass::arfColor);

    m_pass->finishSubpass();

    m_pass->addSubpassDependency({
                                   VK_SUBPASS_EXTERNAL,
                                   0,
                                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                   VK_ACCESS_MEMORY_WRITE_BIT,
                                   VK_ACCESS_SHADER_READ_BIT,
                                   VK_DEPENDENCY_BY_REGION_BIT
                                 });

    m_pass->addSubpassDependency({
                                   0,
                                   VK_SUBPASS_EXTERNAL,
                                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                   VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                   VK_ACCESS_MEMORY_READ_BIT,
                                   0
                                 });

    m_pass->finishRenderPass();
  }

  void GlobalLightStep::setupPipeline(VkExtent2D extent) {
    GraphicsPipelineCreator creator;

    creator.addAttachment({
                            VK_TRUE,
                            VK_BLEND_FACTOR_ONE,
                            VK_BLEND_FACTOR_ONE,
                            VK_BLEND_OP_ADD,
                            VK_BLEND_FACTOR_ONE,
                            VK_BLEND_FACTOR_ZERO,
                            VK_BLEND_OP_ADD,
                            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                            VK_COLOR_COMPONENT_A_BIT
                          });

    creator.setFrontFace(VK_FRONT_FACE_CLOCKWISE);

    creator.setViewport({
                          0,
                          0,
                          static_cast<float>(extent.width),
                          static_cast<float>(extent.height),
                          0,
                          1
                        });

    creator.setScissor({
                         {0, 0},
                         extent
                       });

    creator.setShaderStages({m_vertexShader->getCreateInfo(), m_fragmentShader->getCreateInfo()});

    m_pipeline = util::make_ptr<GraphicsPipeline>(
                                                  creator.finishCreate(getOwningDevice(), m_layout, *m_pass, 0, true)
                                                 );
  }

  void GlobalLightStep::updateDescriptorSets(std::vector<ImageView> const&                   gbufferViews,
                                             std::vector<Renderer::ShadowMappedLight> const& lights,
                                             Buffer&                                         cameraUBO,
                                             Buffer&                                         lightsUBO,
                                             VkSampler                                       sampler) const {
    std::vector<VkWriteDescriptorSet>  descriptorWrites;
    std::vector<VkDescriptorImageInfo> imageInfos;

    descriptorWrites.reserve(NUM_EXPECTED_GBUFFER_IMAGES + 2 + 1);
    imageInfos.reserve(NUM_EXPECTED_GBUFFER_IMAGES + MAX_GLOBAL_LIGHTS);

    assert(gbufferViews.size() == NUM_EXPECTED_GBUFFER_IMAGES + 1); // + depth buffer
    for (uint32_t i = 0; i < NUM_EXPECTED_GBUFFER_IMAGES; ++i)
      imageInfos.push_back({sampler, gbufferViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

    assert(lights.size() <= MAX_GLOBAL_LIGHTS);
    for (uint32_t i = 0; i < lights.size(); ++i)
      imageInfos.push_back({
                             sampler,
                             lights[i].m_depthBuffer->getImageViews().front(),
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                           });

    descriptorWrites.push_back({
                                 VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                 nullptr,
                                 m_descriptorSet,
                                 0,
                                 0,
                                 1,
                                 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                 nullptr,
                                 &cameraUBO.getDescriptorInfo(),
                                 nullptr
                               });

    descriptorWrites.push_back({
                                 VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                 nullptr,
                                 m_descriptorSet,
                                 1,
                                 0,
                                 1,
                                 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                 nullptr,
                                 &lightsUBO.getDescriptorInfo(),
                                 nullptr
                               });

    for (uint32_t i = 0; i < NUM_EXPECTED_GBUFFER_IMAGES; ++i) {
      descriptorWrites.push_back({
                                   VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                   nullptr,
                                   m_descriptorSet,
                                   i + 2,
                                   0,
                                   1,
                                   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                   &imageInfos[i],
                                   nullptr,
                                   nullptr
                                 });
    }

    descriptorWrites.push_back({
                                 VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                 nullptr,
                                 m_descriptorSet,
                                 5,
                                 0,
                                 static_cast<uint32_t>(lights.size()),
                                 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                 &imageInfos[NUM_EXPECTED_GBUFFER_IMAGES],
                                 nullptr,
                                 nullptr
                               });

    vkUpdateDescriptorSets(getOwningDevice(),
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(),
                           0,
                           nullptr);
  }

  void GlobalLightStep::writeCmdBuff(Framebuffer& fb, VkRect2D renderArea) const {
    if (renderArea.extent.width == 0)
      renderArea.extent = fb.getExtent();

    VkClearValue clearValue = {{{0}}};

    auto& cmdBuff = m_cmdBuff.get();

    VkRenderPassBeginInfo beginInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      nullptr,
      *m_pass,
      fb,
      renderArea,
      1,
      &clearValue
    };

    cmdBuff.start(false);
    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
      m_layout, 0,
      1, &m_descriptorSet,
      0, nullptr);

    vkCmdBeginRenderPass(cmdBuff, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdDraw(cmdBuff, 4, 1, 0, 0);
    vkCmdEndRenderPass(cmdBuff);
    cmdBuff.end();
  }
}
