// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Render_PostProcessStep.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 12m 11d
// * Last Altered: 2019y 12m 11d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "RenderSteps.h"

namespace dw {
  FinalStep::FinalStep(LogicalDevice& device, CommandPool& pool, uint32_t numSwapchainImages)
    : RenderStep(device),
      m_imageCount(numSwapchainImages) {
    m_cmdBuffs.reserve(m_imageCount);
    for (size_t i = 0; i < m_imageCount; ++i) {
      m_cmdBuffs.emplace_back(pool.allocateCommandBuffer());
    }
  }

  FinalStep::FinalStep(FinalStep&& o) noexcept
    : RenderStep(std::move(o)),
      m_descriptorSets(std::move(o.m_descriptorSets)),
      m_cmdBuffs(std::move(o.m_cmdBuffs)),
      m_vertexShader(std::move(o.m_vertexShader)),
      m_fragmentShader(std::move(o.m_fragmentShader)),
      m_imageCount(o.m_imageCount) {
    o.m_descriptorSets.clear();
    o.m_cmdBuffs.clear();
  }

  CommandBuffer& FinalStep::getCommandBuffer(uint32_t index) {
    return m_cmdBuffs.at(index);
  }

  void FinalStep::setupRenderPass(std::vector<util::Ref<Image>> const& images) {
    assert(images.size() == 1);

    m_pass = util::make_ptr<RenderPass>(getOwningDevice());
    m_pass->addAttachment(images[0].get().getAttachmentDesc(VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                            VK_ATTACHMENT_STORE_OP_STORE,
                                                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
    m_pass->addAttachmentRef(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, RenderPass::arfColor);

    m_pass->finishSubpass();

    m_pass->addSubpassDependency({
                                   VK_SUBPASS_EXTERNAL,
                                   0,
                                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                   VK_ACCESS_MEMORY_WRITE_BIT,
                                   VK_ACCESS_SHADER_READ_BIT,
                                   0
                                 });

#ifdef DW_USE_IMGUI
    m_pass->addAttachmentRef(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, RenderPass::arfColor);
    m_pass->finishSubpass();

    m_pass->addSubpassDependency({
                                   0,
                                   1,
                                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                                   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                                   VK_DEPENDENCY_BY_REGION_BIT
                                 });

    m_pass->addSubpassDependency({
                                   1,
                                   VK_SUBPASS_EXTERNAL,
                                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                   VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                   VK_ACCESS_MEMORY_READ_BIT,
                                   VK_DEPENDENCY_BY_REGION_BIT
                                 });
#else
    m_pass->addSubpassDependency({
                                   0,
                                   VK_SUBPASS_EXTERNAL,
                                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                   VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                   VK_ACCESS_MEMORY_READ_BIT,
                                   VK_DEPENDENCY_BY_REGION_BIT
      });
#endif

    m_pass->finishRenderPass();
  }

  void FinalStep::setupDescriptors() {
    VkDescriptorSetLayoutBinding binding = {
      0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
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
      throw std::runtime_error("Could not create post processing descriptor set layout");

    VkDescriptorPoolSize poolSize = {
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      m_imageCount
    };

    VkDescriptorPoolCreateInfo poolCreate = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      nullptr,
      0,
      m_imageCount,
      1,
      &poolSize
    };

    if (vkCreateDescriptorPool(getOwningDevice(), &poolCreate, nullptr, &m_descriptorPool) != VK_SUCCESS)
      throw std::runtime_error("Could not create post processing descriptor pool");

    std::vector<VkDescriptorSetLayout> layouts = { m_imageCount, m_descSetLayout };
    VkDescriptorSetAllocateInfo allocateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      nullptr,
      m_descriptorPool,
      m_imageCount,
      layouts.data()
    };

    m_descriptorSets.resize(m_imageCount);

    if (vkAllocateDescriptorSets(getOwningDevice(), &allocateInfo, m_descriptorSets.data()) != VK_SUCCESS)
      throw std::runtime_error("Could not allocate post processing descriptor sets");
  }

  void FinalStep::setupPipeline(VkExtent2D extent) {
    GraphicsPipelineCreator creator;
    creator.addAttachment({
                            VK_FALSE, // This is disabled because transparency is not currently allowed during the geometry pass.
                            VK_BLEND_FACTOR_SRC_ALPHA,
                            VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                            VK_BLEND_OP_ADD,
                            VK_BLEND_FACTOR_SRC_ALPHA,
                            VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                            VK_BLEND_OP_ADD,
                            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                            VK_COLOR_COMPONENT_A_BIT
      });

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

    creator.setFrontFace(VK_FRONT_FACE_CLOCKWISE);
    creator.setShaderStages({ m_vertexShader->getCreateInfo(), m_fragmentShader->getCreateInfo() });

    m_pipeline = util::make_ptr<GraphicsPipeline>(
      creator.finishCreate(getOwningDevice(), m_layout, *m_pass, 0, true)
      );
  }

  void FinalStep::setupShaders() {
    m_vertexShader = util::make_ptr<Shader<ShaderStage::Vertex>>(ShaderModule::Load(getOwningDevice(), "fsq_vert.spv"));
    m_fragmentShader = util::make_ptr<Shader<ShaderStage::Fragment>>(ShaderModule::Load(getOwningDevice(),
      "fxaa_frag.spv"));
  }

  void FinalStep::writeCmdBuff(std::vector<Framebuffer> const& fbs,
                               Image const&                    previousImage,
                               VkRect2D                        renderArea,
                               uint32_t                        image) {
    const auto count = m_imageCount;

    if (renderArea.extent.width == 0)
      renderArea.extent = fbs.front().getExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0, 0, 0, 0}};


    auto writeCmdBuff = [&, this](uint32_t i, bool renderImGui) {
      auto& commandBuffer = m_cmdBuffs[i].get();

      if (commandBuffer.canBeReset())
        commandBuffer.reset();

      VkRenderPassBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        nullptr,
        *m_pass,
        fbs[i],
        renderArea,
        static_cast<uint32_t>(clearValues.size()),
        clearValues.data()
      };

      VkImageMemoryBarrier barrier = {
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
      };

      commandBuffer.start(false);
      vkCmdPipelineBarrier(commandBuffer,
                           VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           0,
                           0,
                           nullptr,
                           0,
                           nullptr,
                           1,
                           &barrier);
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

#ifdef DW_USE_IMGUI
      vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
      if (renderImGui) {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
      }
#endif
      vkCmdEndRenderPass(commandBuffer);

      commandBuffer.end();
    };
    if (image == ~0u)
      for (size_t i = 0; i < count; ++i)
        writeCmdBuff(i, false);
    else {
#ifdef DW_USE_IMGUI
      ImGui::Render();
#endif
      writeCmdBuff(image, true);
    }
  }

  void FinalStep::updateDescriptorSets(
    ImageView const& previousImage,
    VkSampler        sampler) {
    uint32_t numSampledImages = 1;//NUM_EXPECTED_GBUFFER_IMAGES + 1;

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    descriptorWrites.reserve(m_descriptorSets.size() * (1));

    std::vector<VkDescriptorImageInfo> imageInfos;
    imageInfos.reserve(numSampledImages);

    imageInfos.push_back({sampler, previousImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

    for (auto& set : m_descriptorSets) {
      /*descriptorWrites.push_back({
                                   VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                   nullptr,
                                   set,
                                   0,
                                   0,
                                   1,
                                   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                   nullptr,
                                   &cameraUBO.getDescriptorInfo(),
                                   nullptr
        });*/

      /*descriptorWrites.push_back({
                                   VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                   nullptr,
                                   set,
                                   1,
                                   0,
                                   1,
                                   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                   nullptr,
                                   &shaderControlUBO.getDescriptorInfo(),
                                   nullptr
        });*/

      for (uint32_t j = 0; j < numSampledImages; ++j) {
        descriptorWrites.push_back({
                                     VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                     nullptr,
                                     set,
                                     j,
                                     0,
                                     1,
                                     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                     &imageInfos[j],
                                     nullptr,
                                     nullptr
                                   });
      }

      /*descriptorWrites.push_back({
                                   VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                   nullptr,
                                   set,
                                   6,
                                   0,
                                   1,
                                   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                   nullptr,
                                   &lightsUBO.getDescriptorInfo(),
                                   nullptr
        });*/
    }

    vkUpdateDescriptorSets(getOwningDevice(),
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(),
                           0,
                           nullptr);
  }

}
