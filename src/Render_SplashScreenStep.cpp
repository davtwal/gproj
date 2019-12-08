// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Render_SplashScreenStep.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 12m 08d
// * Last Altered: 2019y 12m 08d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "RenderSteps.h"

namespace dw {
  SplashScreenStep::SplashScreenStep(LogicalDevice& device, CommandPool& pool, uint32_t imageCount)
    : RenderStep(device),
      m_imageCount(imageCount) {
    m_cmdBuffs.reserve(m_imageCount);
    for (size_t i = 0; i < m_imageCount; ++i) {
      m_cmdBuffs.emplace_back(pool.allocateCommandBuffer());
    }
  }

  SplashScreenStep::SplashScreenStep(SplashScreenStep&& o) noexcept
    : RenderStep(std::move(o)),
      m_descriptorSets(std::move(o.m_descriptorSets)),
      m_cmdBuffs(std::move(o.m_cmdBuffs)),
      m_vertexShader(std::move(o.m_vertexShader)),
      m_fragmentShader(std::move(o.m_fragmentShader)),
      m_imageCount(o.m_imageCount) {
    o.m_descriptorSets.clear();
    o.m_cmdBuffs.clear();
  }

  void SplashScreenStep::setupRenderPass(std::vector<util::Ref<Image>> const& images) {
    m_pass = util::make_ptr<RenderPass>(getOwningDevice());
    m_pass->addAttachment(images[0].get().getAttachmentDesc(VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                            VK_ATTACHMENT_STORE_OP_STORE,
                                                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
    m_pass->addAttachmentRef(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, RenderPass::arfColor);

    m_pass->finishSubpass();

    m_pass->addSubpassDependency({
                                   0,
                                   VK_SUBPASS_EXTERNAL,
                                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                   VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                   VK_ACCESS_MEMORY_READ_BIT,
                                   VK_DEPENDENCY_BY_REGION_BIT
                                 });

    m_pass->finishRenderPass();
  }

  void SplashScreenStep::setupPipeline(VkExtent2D extent) {
    GraphicsPipelineCreator creator;
    creator.addAttachment({
                            VK_FALSE,
                            VK_BLEND_FACTOR_ONE,
                            VK_BLEND_FACTOR_ONE,
                            VK_BLEND_OP_ADD,
                            VK_BLEND_FACTOR_ONE,
                            VK_BLEND_FACTOR_ONE,
                            VK_BLEND_OP_ADD,
                            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                            VK_COLOR_COMPONENT_A_BIT
      });

    // might have to flip
    creator.setViewport({
                          0,
                          static_cast<float>(extent.height),
                          static_cast<float>(extent.width),
                          -static_cast<float>(extent.height),
                          0,
                          1
      });

    creator.setScissor({
                         {0, 0},
                         extent
      });

    creator.setFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
    creator.setShaderStages({ m_vertexShader->getCreateInfo(), m_fragmentShader->getCreateInfo() });

    m_pipeline = util::make_ptr<GraphicsPipeline>(
      creator.finishCreate(getOwningDevice(), m_layout, *m_pass, 0, true)
      );
  }

  void SplashScreenStep::setupDescriptors() {
    VkDescriptorSetLayoutBinding binding = {
      0,
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      1,
      VK_SHADER_STAGE_FRAGMENT_BIT,
      nullptr
    };

    VkDescriptorSetLayoutCreateInfo layoutCreate = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      1,
      &binding
    };

    if (vkCreateDescriptorSetLayout(getOwningDevice(), &layoutCreate, nullptr, &m_descSetLayout) != VK_SUCCESS)
      throw std::runtime_error("Could not create descriptor set layout for splash screen");

    VkDescriptorPoolSize poolSize = {
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      m_imageCount
    };

    VkDescriptorPoolCreateInfo poolInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      nullptr,
      0,
      m_imageCount, 1, &poolSize
    };

    if (vkCreateDescriptorPool(getOwningDevice(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
      throw std::runtime_error("Could not create descriptor pool for splash screen");

    std::vector<VkDescriptorSetLayout> layouts = { m_imageCount, m_descSetLayout };
    VkDescriptorSetAllocateInfo descAllocInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      nullptr,
      m_descriptorPool,
      m_imageCount,
      layouts.data()
    };

    m_descriptorSets.resize(m_imageCount);
    VkResult result = vkAllocateDescriptorSets(getOwningDevice(), &descAllocInfo, m_descriptorSets.data());
    switch (result) {
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      throw std::runtime_error("could not allocate descriptor sets (splash) (out of host memory)");

    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      throw std::runtime_error("could not allocate descriptor sets (splash) (out of device memory)");

    case VK_ERROR_FRAGMENTED_POOL:
      throw std::runtime_error("could not allocate descriptor sets (splash) (fragmented pool)");

    case VK_ERROR_OUT_OF_POOL_MEMORY:
      throw std::runtime_error("could not allocate descriptor sets (splash) (out of pool memory)");

    default:
      break;
    }
  }

  void SplashScreenStep::writeCmdBuff(uint32_t i, Framebuffer const& fb, VkRect2D renderArea) const {
    if (renderArea.extent.width == 0)
      renderArea.extent = fb.getExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0, 0, 0, 0} };

    auto& commandBuffer = m_cmdBuffs[i].get();

    if (commandBuffer.canBeReset())
      commandBuffer.reset();

    VkRenderPassBeginInfo beginInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      nullptr,
      *m_pass,
      fb,
      renderArea,
      static_cast<uint32_t>(clearValues.size()),
      clearValues.data()
    };

    commandBuffer.start(false);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
    vkCmdBindDescriptorSets(commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      m_layout,
      0,
      1,
      &m_descriptorSets[i],
      0,
      nullptr);
    vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
    commandBuffer.end();
  }

  void SplashScreenStep::updateDescriptorSets(uint32_t index, ImageView& logoView, VkSampler sampler) const {
    VkDescriptorImageInfo imageInfo = {
      sampler,
      logoView,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    descriptorWrites.reserve(1);//m_descriptorSets.size());

    //for(auto& set : m_descriptorSets) {
      descriptorWrites.push_back({
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        nullptr,
        m_descriptorSets[index],//set,
        0,
        0,
        1,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        &imageInfo,
        nullptr,
        nullptr
      });
    //}

    vkUpdateDescriptorSets(getOwningDevice(),
      static_cast<uint32_t>(descriptorWrites.size()),
      descriptorWrites.data(),
      0,
      nullptr);
  }

  CommandBuffer& SplashScreenStep::getCommandBuffer(uint32_t nextImageIndex) const {
    return m_cmdBuffs.at(nextImageIndex);
  }

  void SplashScreenStep::setupShaders() {
    m_vertexShader = util::make_ptr<Shader<ShaderStage::Vertex>
    >(ShaderModule::Load(getOwningDevice(), "fsq_vert.spv"));
    m_fragmentShader = util::make_ptr<Shader<ShaderStage::Fragment>>(ShaderModule::Load(getOwningDevice(),
                                                                                        "logo_display_frag.spv"));
  }


}
