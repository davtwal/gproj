// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Render_AmbientStep.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 12m 14d
// * Last Altered: 2019y 12m 14d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "render/RenderSteps.h"

namespace dw {
  AmbientStep::AmbientStep(LogicalDevice& device, CommandPool& pool)
    : RenderStep(device),
    m_cmdBuff(pool.allocateCommandBuffer()) {
  }

  AmbientStep::AmbientStep(AmbientStep&& o) noexcept
    : RenderStep(std::move(o)),
    m_vertexShader(std::move(o.m_vertexShader)),
    m_fragmentShader(std::move(o.m_fragmentShader)),
    m_cmdBuff(o.m_cmdBuff),
    m_descriptorSet(o.m_descriptorSet) {
    o.m_vertexShader = nullptr;
    o.m_fragmentShader = nullptr;
    o.m_descriptorSet = nullptr;
  }

  CommandBuffer& AmbientStep::getCommandBuffer() const {
    return m_cmdBuff;
  }

  void AmbientStep::setupShaders() {
    m_vertexShader = util::make_ptr<Shader<ShaderStage::Vertex>>(
      ShaderModule::Load(getOwningDevice(),
        "fsq_vert.spv"
      ));

    m_fragmentShader = util::make_ptr<Shader<ShaderStage::Fragment>>(
      ShaderModule::Load(getOwningDevice(),
        "ambient_occlusion_frag.spv"
      ));
  }

  void AmbientStep::setupDescriptors() {
    std::array<VkDescriptorSetLayoutBinding, 2> bindings;
    bindings[0] = {
      0,
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      1,
      VK_SHADER_STAGE_FRAGMENT_BIT,
      nullptr
    };
    bindings[1] = {
      1,
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      1,
      VK_SHADER_STAGE_FRAGMENT_BIT,
      nullptr
    };

    VkDescriptorSetLayoutCreateInfo layoutCreate = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(bindings.size()),
      bindings.data()
    };

    if (vkCreateDescriptorSetLayout(getOwningDevice(), &layoutCreate, nullptr, &m_descSetLayout) != VK_SUCCESS)
      throw std::runtime_error("could not create ambient descriptor set");

    VkDescriptorPoolSize size = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 };
    VkDescriptorPoolCreateInfo poolInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      nullptr,
      0,
      1,
      1,
      &size
    };
    if (vkCreateDescriptorPool(getOwningDevice(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
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

  void AmbientStep::setupRenderPass(std::vector<util::Ref<Image>> const& images) {
    assert(images.size() == 1);

    m_pass = util::make_ptr<RenderPass>(getOwningDevice());
    m_pass->addAttachment(images[0].get().getAttachmentDesc(VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
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

  void AmbientStep::setupPipeline(VkExtent2D extent) {
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

    creator.setShaderStages({ m_vertexShader->getCreateInfo(), m_fragmentShader->getCreateInfo() });

    m_pipeline = util::make_ptr<GraphicsPipeline>(
      creator.finishCreate(getOwningDevice(), m_layout, *m_pass, 0, true)
      );
  }

  void AmbientStep::writeCmdBuff(Framebuffer const& fb, VkRect2D renderArea) {
    if (renderArea.extent.width == 0)
      renderArea.extent = fb.getExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0, 0, 0, 0} };

    auto& commandBuffer = m_cmdBuff.get();//s[i].get();

    if (commandBuffer.canBeReset())
      commandBuffer.reset();

    VkRenderPassBeginInfo begin = {
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      nullptr,
      *m_pass,
      fb,
      renderArea,
      static_cast<uint32_t>(clearValues.size()),
      clearValues.data()
    };

    /*VkImageMemoryBarrier barrier = {
     VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
     nullptr,
     VK_ACCESS_MEMORY_WRITE_BIT,
     VK_ACCESS_MEMORY_READ_BIT,
     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
     VK_QUEUE_FAMILY_IGNORED,
     VK_QUEUE_FAMILY_IGNORED,
     previousImage,
     {
       VK_IMAGE_ASPECT_COLOR_BIT,
       0,
       1,
       0,
       1
     }
    };*/

    commandBuffer.start(false);
    //vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
    //  1, &barrier);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
    vkCmdBindDescriptorSets(commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      m_layout,
      0,
      1,
      &m_descriptorSet,
      0,
      nullptr);
    vkCmdBeginRenderPass(commandBuffer, &begin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    commandBuffer.end();
  }

  void AmbientStep::updateDescriptorSets(ImageView const& gbuffPosition, ImageView const& gbuffNormal, VkSampler sampler) {
    uint32_t numSampledImages = NUM_EXPECTED_GBUFFER_IMAGES - 1;

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    descriptorWrites.reserve(/*m_descriptorSets.size() */(NUM_EXPECTED_GBUFFER_IMAGES + 3));

    std::vector<VkDescriptorImageInfo> imageInfos;
    imageInfos.reserve(numSampledImages);

    imageInfos.push_back({ sampler, gbuffPosition, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
    imageInfos.push_back({ sampler, gbuffNormal, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

    for (uint32_t j = 0; j < numSampledImages; ++j) {
      descriptorWrites.push_back({
                                   VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                   nullptr,
                                   m_descriptorSet,
                                   j,
                                   0,
                                   1,
                                   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                   &imageInfos[j],
                                   nullptr,
                                   nullptr
        });
    }

    vkUpdateDescriptorSets(getOwningDevice(),
      static_cast<uint32_t>(descriptorWrites.size()),
      descriptorWrites.data(),
      0,
      nullptr);
  }

}