// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Render_BlurStep.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 10m 20d
// * Last Altered: 2019y 11m 20d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "render/RenderSteps.h"
#include <stdexcept>
#include <array>

namespace dw {
  static constexpr uint32_t KERNEL_SIZE = 15;

  BlurStep::BlurStep(LogicalDevice& device, CommandPool& pool)
    : RenderStep(device),
      m_cmdBuff(pool.allocateCommandBuffer()) {
  }

  BlurStep::BlurStep(BlurStep&& o) noexcept
    : RenderStep(std::move(o)),
      m_blur_x(std::move(o.m_blur_x)),
      m_blur_y(std::move(o.m_blur_y)),
      m_cmdBuff(o.m_cmdBuff),
      m_compute_x(o.m_compute_x),
      m_compute_y(o.m_compute_y),
      m_descriptorSets(std::move(o.m_descriptorSets)) {
    o.m_blur_x.reset();
    o.m_blur_y.reset();
    o.m_compute_x = nullptr;
    o.m_compute_y = nullptr;
    o.m_descriptorSets.clear();
  }

  BlurStep::~BlurStep() {
    if (m_compute_x) {
      vkDestroyPipeline(getOwningDevice(), m_compute_x, nullptr);
      m_compute_x = nullptr;
    }

    if (m_compute_y) {
      vkDestroyPipeline(getOwningDevice(), m_compute_y, nullptr);
      m_compute_y = nullptr;
    }
  }


  void BlurStep::setupShaders() {
    m_blur_x = util::make_ptr<Shader<ShaderStage::Compute>>(
                                                            ShaderModule::Load(getOwningDevice(),
                                                                               "blur_x_comp.spv"
                                                                              ));

    m_blur_y = util::make_ptr<Shader<ShaderStage::Compute>>(
                                                            ShaderModule::Load(getOwningDevice(),
                                                                               "blur_y_comp.spv"
                                                                              ));
  }

  void BlurStep::setupRenderPass(std::vector<util::Ref<Image>> const&) {
  }

  void BlurStep::setupPipelineLayout(VkPipelineLayout layout) {
    VkPushConstantRange range = {
      VK_SHADER_STAGE_COMPUTE_BIT,
      0,
      sizeof(float) * KERNEL_SIZE
    };

    VkPipelineLayoutCreateInfo layoutCreate = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      1,
      &m_descSetLayout,
      1,
      &range
    };

    if (vkCreatePipelineLayout(getOwningDevice(), &layoutCreate, nullptr, &m_layout) != VK_SUCCESS)
      throw std::runtime_error("Could not create compute pipeline layout");
  }

  void BlurStep::setupPipeline(VkExtent2D extent) {
    VkSpecializationMapEntry mapEntries[2] = {
      {0, 0, sizeof(int)},
      {1, sizeof(int), sizeof(int)}
    };

    int                  data[2]  = {KERNEL_SIZE, KERNEL_SIZE / 2};
    VkSpecializationInfo specInfo = {
      2,
      mapEntries,
      sizeof(int) * 2,
      data
    };

    VkComputePipelineCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      nullptr,
      0,
      {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr,
        0,
        VK_SHADER_STAGE_COMPUTE_BIT,
        m_blur_x->getCreateInfo().module,
        "main",
        &specInfo
      },
      m_layout,
      nullptr,
      -1
    };

    if (vkCreateComputePipelines(getOwningDevice(), nullptr, 1, &createInfo, nullptr, &m_compute_x) != VK_SUCCESS)
      throw std::runtime_error("Could not create compute blur X pipeline");

    createInfo.stage.module = m_blur_y->getCreateInfo().module;
    if (vkCreateComputePipelines(getOwningDevice(), nullptr, 1, &createInfo, nullptr, &m_compute_y) != VK_SUCCESS)
      throw std::runtime_error("Could not create compute blur Y pipeline");
  }

  void BlurStep::setupDescriptors() {
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
      {
        0,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        1,
        VK_SHADER_STAGE_COMPUTE_BIT,
        nullptr
      },
      {
        1,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        1,
        VK_SHADER_STAGE_COMPUTE_BIT,
        nullptr
      }
    };

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      nullptr,
      0, // VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR for push descriptor
      static_cast<uint32_t>(layoutBindings.size()),
      layoutBindings.data()
    };

    if (vkCreateDescriptorSetLayout(getOwningDevice(), &layoutCreateInfo, nullptr, &m_descSetLayout) != VK_SUCCESS || !
        m_descSetLayout)
      throw std::runtime_error("Could not create descriptor set layout");

    std::vector<VkDescriptorPoolSize> poolSizes = {
      {
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        4 * GlobalLightStep::MAX_GLOBAL_LIGHTS
      }
    };

    VkDescriptorPoolCreateInfo poolCreateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      nullptr,
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      2 * GlobalLightStep::MAX_GLOBAL_LIGHTS,
      static_cast<uint32_t>(poolSizes.size()),
      poolSizes.data()
    };

    if (vkCreateDescriptorPool(getOwningDevice(), &poolCreateInfo, nullptr, &m_descriptorPool) != VK_SUCCESS || !
        m_descriptorPool)
      throw std::runtime_error("Could not create descriptor pool");
  }

  CommandBuffer& BlurStep::getCommandBuffer() const {
    return m_cmdBuff;
  }

  void BlurStep::updateDescriptorSets(DescriptorSetCont const& set,
                                      ImageView&               source,
                                      ImageView&               intermediate,
                                      ImageView&               dest,
                                      VkSampler                sampler) const {
    std::vector<VkWriteDescriptorSet> writes;
    writes.reserve(4);

    VkDescriptorImageInfo sourceInfo = {
      sampler,
      source,
      VK_IMAGE_LAYOUT_GENERAL
    };

    VkDescriptorImageInfo intermediateInfo = {
      sampler,
      intermediate,
      VK_IMAGE_LAYOUT_GENERAL
    };

    VkDescriptorImageInfo destInfo = {
      sampler,
      dest,
      VK_IMAGE_LAYOUT_GENERAL
    };

    VkWriteDescriptorSet write = {
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      nullptr,
      set.x,
      0,
      0,
      1,
      VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      &sourceInfo,
      nullptr,
      nullptr
    };

    writes.push_back(write);

    write.dstBinding = 1;
    write.pImageInfo = &intermediateInfo;
    writes.push_back(write);

    write.dstSet     = set.y;
    write.dstBinding = 0;
    writes.push_back(write);

    write.pImageInfo = &destInfo;
    write.dstBinding = 1;
    writes.push_back(write);

    vkUpdateDescriptorSets(getOwningDevice(), writes.size(), writes.data(), 0, nullptr);
  }

  void BlurStep::writeCmdBuff(std::vector<Renderer::ShadowMappedLight> const& lights,
                              DependentImage&                                 intermediateImg,
                              ImageView&                                      intermediaryView) {
    assert(lights.size() <= GlobalLightStep::MAX_GLOBAL_LIGHTS);

    if(!m_descriptorSets.empty())
      vkFreeDescriptorSets(getOwningDevice(), m_descriptorPool, m_descriptorSets.size() * 2, &m_descriptorSets.front().x);

    m_descriptorSets.clear();
    m_descriptorSets.resize(lights.size());

    std::vector<VkDescriptorSetLayout> layouts = {
      2 * lights.size(),
      m_descSetLayout
    };
    VkDescriptorSetAllocateInfo descSetAllocInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      nullptr,
      m_descriptorPool,
      2 * lights.size(),
      layouts.data()
    };

    // note: because m_descriptorSet_y comes right after m_descriptorSet_x, then the allocation for 2
    // puts the second into _y
    // and also all of these descriptors are nice and packed :)
    VkResult result = vkAllocateDescriptorSets(getOwningDevice(),
      &descSetAllocInfo,
      &m_descriptorSets.front().x);
    switch (result) {
    case VK_ERROR_OUT_OF_POOL_MEMORY:
      throw std::runtime_error("Could not allocate blur descriptor sets: Out of pool memory");
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      throw std::runtime_error("Could not allocate blur descriptor sets: out of host memory");
    case VK_ERROR_FRAGMENTED_POOL:
      throw std::runtime_error("Could not allocate blur descriptor sets: fragmented pool");
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      throw std::runtime_error("Could not allocate blur descriptor sets: out of device memory");
    default: break;
    }

    std::array<float, KERNEL_SIZE> weights{{0}};

    float weightSum = 0.f;
    for (uint32_t i = 0; i < KERNEL_SIZE; ++i) {
      // integer division on purpose
      constexpr int w = KERNEL_SIZE / 2;
      int           x = i - w;
      float         s = x / (static_cast<float>(w) / 2);
      weights[i]      = glm::exp(-.5f * s * s);
      weightSum += weights[i];
    }

    for (auto& w : weights) {
      w /= weightSum;
    }

    VkImageMemoryBarrier barrier = {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      nullptr,
      VK_ACCESS_SHADER_WRITE_BIT,
      VK_ACCESS_SHADER_READ_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_GENERAL,
      VK_QUEUE_FAMILY_IGNORED,
      VK_QUEUE_FAMILY_IGNORED,
      nullptr,
      {
        VK_IMAGE_ASPECT_COLOR_BIT,
        0,
        1,
        0,
        1
      }
    };

    std::array<VkImageMemoryBarrier, 2> barriers{barrier, barrier};

    barriers[0].image = intermediateImg;

    auto& cmdBuff = m_cmdBuff.get();
    cmdBuff.start(false);

    vkCmdPipelineBarrier(cmdBuff,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barriers[0]);
    // ey broski, you were in the middle of making another compute pipeline to do the Y blur :>
    barriers[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barriers[1].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    const auto size = intermediateImg.getSize();
    for (size_t i = 0; i < lights.size(); ++i) {
      auto& light       = lights.at(i);
      auto& view        = light.m_depthBuffer->getImageViews().front();
      auto& image       = light.m_depthBuffer->getImages().front();
      auto& descriptors = m_descriptorSets.at(i);
      updateDescriptorSets(descriptors, view, intermediaryView, view);
      barriers[1].image = image;

      vkCmdBindPipeline(cmdBuff,
                        VK_PIPELINE_BIND_POINT_COMPUTE,
                        m_compute_x);
      vkCmdBindDescriptorSets(cmdBuff,
                              VK_PIPELINE_BIND_POINT_COMPUTE,
                              m_layout,
                              0,
                              1,
                              &descriptors.x,
                              0,
                              nullptr);
      vkCmdPipelineBarrier(cmdBuff,
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0,
                           0,
                           nullptr,
                           0,
                           nullptr,
                           1,
                           &barriers[1]);
      vkCmdPushConstants(cmdBuff,
                         m_layout,
                         VK_SHADER_STAGE_COMPUTE_BIT,
                         0,
                         sizeof(float) * KERNEL_SIZE,
                         weights.data());
      vkCmdDispatch(cmdBuff,
                    (size.width / 128) + size.width % 128,
                    size.height,
                    1);

      barriers[1].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
      vkCmdPipelineBarrier(cmdBuff,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0,
                           0,
                           nullptr,
                           0,
                           nullptr,
                           static_cast<uint32_t>(barriers.size()),
                           barriers.data());

      vkCmdBindPipeline(cmdBuff,
                        VK_PIPELINE_BIND_POINT_COMPUTE,
                        m_compute_y);
      vkCmdBindDescriptorSets(cmdBuff,
                              VK_PIPELINE_BIND_POINT_COMPUTE,
                              m_layout,
                              0,
                              1,
                              &descriptors.y,
                              0,
                              nullptr);
      vkCmdPushConstants(cmdBuff,
                         m_layout,
                         VK_SHADER_STAGE_COMPUTE_BIT,
                         0,
                         sizeof(float) * KERNEL_SIZE,
                         weights.data());
      vkCmdDispatch(cmdBuff,
                    size.width,
                    (size.height / 128) + size.height % 128,
                    1);

      barriers[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      vkCmdPipelineBarrier(cmdBuff,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           0,
                           0,
                           nullptr,
                           0,
                           nullptr,
                           1,
                           &barriers[1]);
      barriers[1].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barriers[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    cmdBuff.end();
  }


}
