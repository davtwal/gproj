// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Render_LocalLightingStep.cpp
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

  void FinalStep::setupDescriptors() {
    // one sampler per gbuffer image + one sampler for previous image
    uint32_t numSampledImages = NUM_EXPECTED_GBUFFER_IMAGES + 1;

    std::vector<VkDescriptorSetLayoutBinding> finalBindings;
    finalBindings.resize(numSampledImages + 2);
    // one view eye/view dir UBO +
    // one for light UBO

    finalBindings.front() = {
      0,
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      1,
      VK_SHADER_STAGE_FRAGMENT_BIT,
      nullptr
    };

    for (uint32_t i = 1; i < finalBindings.size(); ++i) {
      finalBindings[i] = {
        i,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        1,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        nullptr
      };
    }

    finalBindings.back() = {
      5,
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      1,
      VK_SHADER_STAGE_FRAGMENT_BIT,
      nullptr
    };

    VkDescriptorSetLayoutCreateInfo finalLayoutCreate = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(finalBindings.size()),
      finalBindings.data()
    };

    if (vkCreateDescriptorSetLayout(getOwningDevice(), &finalLayoutCreate, nullptr, &m_descSetLayout) != VK_SUCCESS)
      throw std::runtime_error("could not create final descriptor set");

    uint32_t                          numImages = m_imageCount;
    std::vector<VkDescriptorPoolSize> finalPoolSizes = {
      {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        numImages * 2
      },
      {
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        numImages * numSampledImages
      }
    };

    VkDescriptorPoolCreateInfo finalPoolInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      nullptr,
      0,
      numImages,
      static_cast<uint32_t>(finalPoolSizes.size()),
      finalPoolSizes.data()
    };

    if (vkCreateDescriptorPool(getOwningDevice(), &finalPoolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
      throw std::runtime_error("could not create final descrition pool");

    std::vector<VkDescriptorSetLayout> finalLayouts = { numImages, m_descSetLayout };
    VkDescriptorSetAllocateInfo        descSetAllocInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      nullptr,
      m_descriptorPool,
      numImages,
      finalLayouts.data()
    };

    m_descriptorSets.resize(numImages);
    VkResult result = vkAllocateDescriptorSets(getOwningDevice(), &descSetAllocInfo, m_descriptorSets.data());
    switch (result) {
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      throw std::runtime_error("could not allocate descriptor sets (final) (out of host memory)");

    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      throw std::runtime_error("could not allocate descriptor sets (final) (out of device memory)");

    case VK_ERROR_FRAGMENTED_POOL:
      throw std::runtime_error("could not allocate descriptor sets (final) (fragmented pool)");

    case VK_ERROR_OUT_OF_POOL_MEMORY:
      throw std::runtime_error("could not allocate descriptor sets (final) (out of pool memory)");

    default:
      break;
    }
  }

  void FinalStep::setupShaders() {
    m_vertexShader = util::make_ptr<Shader<ShaderStage::Vertex>
    >(ShaderModule::Load(getOwningDevice(), "fsq_vert.spv"));
    m_fragmentShader = util::make_ptr<Shader<ShaderStage::Fragment>>(ShaderModule::Load(getOwningDevice(),
      "local_lighting_frag.spv"));
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

  void FinalStep::writeCmdBuff(std::vector<Framebuffer> const& fbs, Image const& previousImage, VkRect2D renderArea) {
    const auto count = m_imageCount;

    if (renderArea.extent.width == 0)
      renderArea.extent = fbs.front().getExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0, 0, 0, 0} };

    for (size_t i = 0; i < count; ++i) {
      auto& commandBuffer = m_cmdBuffs[i].get();

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
      vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
        1, &barrier);
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
  }

  void FinalStep::updateDescriptorSets(std::vector<ImageView> const& gbufferViews,
                                       ImageView const& previousImage,
                                       Buffer& cameraUBO,
                                       Buffer& lightsUBO,
                                       VkSampler                     sampler) {
    uint32_t numSampledImages = NUM_EXPECTED_GBUFFER_IMAGES + 1;

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    descriptorWrites.reserve(m_descriptorSets.size() * (NUM_EXPECTED_GBUFFER_IMAGES + 2));
    
    std::vector<VkDescriptorImageInfo> imageInfos;
    imageInfos.reserve(numSampledImages);

    assert(gbufferViews.size() == NUM_EXPECTED_GBUFFER_IMAGES + 1); // + depth buffer
    for (uint32_t i = 0; i < NUM_EXPECTED_GBUFFER_IMAGES; ++i) {
      imageInfos.push_back({ sampler, gbufferViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
    }

    imageInfos.push_back({ sampler, previousImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

    for (auto& set : m_descriptorSets) {
      descriptorWrites.push_back({
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
        });

      for (uint32_t j = 0; j < numSampledImages; ++j) {
        descriptorWrites.push_back({
                                     VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                     nullptr,
                                     set,
                                     j + 1,
                                     0,
                                     1,
                                     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                     &imageInfos[j],
                                     nullptr,
                                     nullptr
          });
      }

      descriptorWrites.push_back({
                                   VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                   nullptr,
                                   set,
                                   5,
                                   0,
                                   1,
                                   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                   nullptr,
                                   &lightsUBO.getDescriptorInfo(),
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