// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Render_GeometryStep.cpp
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
  GeometryStep::GeometryStep(LogicalDevice& device, CommandPool& pool)
    : RenderStep(device),
    m_cmdBuff(pool.allocateCommandBuffer()) {
  }

  GeometryStep::GeometryStep(GeometryStep&& o) noexcept
    : RenderStep(std::move(o)),
    m_vertexShader(std::move(o.m_vertexShader)),
    m_fragmentShader(std::move(o.m_fragmentShader)),
    m_cmdBuff(o.m_cmdBuff),
    m_descriptorSet(o.m_descriptorSet) {
    o.m_vertexShader = nullptr;
    o.m_fragmentShader = nullptr;
    o.m_descriptorSet = nullptr;
  }

  CommandBuffer& GeometryStep::getCommandBuffer() const {
    return m_cmdBuff;
  }

  void GeometryStep::writeCmdBuff(Framebuffer& fb,
    Renderer::SceneContainer const& scene,
    uint32_t                        alignment,
    VkRect2D                        renderArea) const {
    // 1: deferred pass
    if (renderArea.extent.width == 0) {
      renderArea.extent = fb.getExtent();
    }

    if (!scene.empty()) {
      std::array<VkClearValue, NUM_EXPECTED_GBUFFER_IMAGES + 1> clearValues{};
      clearValues.back().depthStencil = { 1.f, 0 };

      for (uint32_t i = 0; i < NUM_EXPECTED_GBUFFER_IMAGES; ++i) {
        clearValues[i].color = { {0} };
      }

      auto& commandBuff = m_cmdBuff.get();

      commandBuff.start(false);

      VkRenderPassBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        nullptr,
        *m_pass,
        fb,
        renderArea,
        static_cast<uint32_t>(clearValues.size()),
        clearValues.data()
      };

      vkCmdBindPipeline(commandBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);

      renderScene(commandBuff, beginInfo, scene, alignment, m_layout, m_descriptorSet);

      commandBuff.end();
    }

  }

  void GeometryStep::setupDescriptors() {
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
      {
        0,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        1,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        nullptr
      },
      {
        1,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        1,
        VK_SHADER_STAGE_VERTEX_BIT,
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

    ///////////////////////////////////////////////////////
    // POOL AND SETS

    // create uniform buffers - one for each swapchain image
    std::vector<VkDescriptorPoolSize> poolSizes = {
      {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        1
      },
      {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        1
      }
    };

    VkDescriptorPoolCreateInfo poolCreateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      nullptr,
      0,
      1,
      static_cast<uint32_t>(poolSizes.size()),
      poolSizes.data()
    };

    if (vkCreateDescriptorPool(getOwningDevice(), &poolCreateInfo, nullptr, &m_descriptorPool) != VK_SUCCESS || !
      m_descriptorPool)
      throw std::runtime_error("Could not create descriptor pool");

    //////////////////
    // SETS

    VkDescriptorSetAllocateInfo descSetAllocInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      nullptr,
      m_descriptorPool,
      1,
      &m_descSetLayout
    };

    if (vkAllocateDescriptorSets(getOwningDevice(), &descSetAllocInfo, &m_descriptorSet) != VK_SUCCESS)
      throw std::runtime_error("Could not allocate descriptor sets");
  }

  void GeometryStep::updateDescriptorSets(Buffer& modelUBO, Buffer& cameraUBO) const {
    // Descriptor sets are automatically freed once the pool is freed.
    // They can be individually freed if the pool was created with
    // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT sets
    VkDescriptorBufferInfo modelUBOinfo = modelUBO.getDescriptorInfo();
    modelUBOinfo.range = sizeof(ObjectUniform);

    std::vector<VkWriteDescriptorSet> descriptorWrites = {
      {
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
      },
      {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        nullptr,
        m_descriptorSet,
        1,
        0,
        1,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        nullptr,
        &modelUBOinfo,
        nullptr
      }
    };

    vkUpdateDescriptorSets(getOwningDevice(),
      static_cast<uint32_t>(descriptorWrites.size()),
      descriptorWrites.data(),
      0,
      nullptr);
  }

  void GeometryStep::setupShaders() {
    m_vertexShader = util::make_ptr<Shader<ShaderStage::Vertex>>(
      ShaderModule::Load(getOwningDevice(),
        "object_pass_vert.spv"));
    m_fragmentShader = util::make_ptr<Shader<ShaderStage::Fragment>>(
      ShaderModule::Load(getOwningDevice(),
        "gbuffer_filler_frag.spv"));
  }

  void GeometryStep::setupPipeline(VkExtent2D extent) {
    GraphicsPipelineCreator creator;
    creator.setVertexType(Vertex::GetBindingDescriptions(), Vertex::GetBindingAttributes());

    creator.setViewport({
                          0,
                          static_cast<float>(extent.height),
                          static_cast<float>(extent.width),
                          -static_cast<float>(extent.height),
                          0,
                          1.f
      });

    creator.setScissor({
                         {0, 0},
                         extent
      });

    creator.setFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
    creator.setDepthTesting(true);

    static constexpr uint32_t           GBUFFER_IMAGE_COUNT = 3;
    VkPipelineColorBlendAttachmentState colorAttachmentInfo = {
      VK_FALSE, // This is disabled because transparency is not currently allowed during the geometry pass.
      VK_BLEND_FACTOR_SRC_ALPHA,
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      VK_BLEND_OP_ADD,
      VK_BLEND_FACTOR_SRC_ALPHA,
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      VK_BLEND_OP_ADD,
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments{ GBUFFER_IMAGE_COUNT, colorAttachmentInfo };
    creator.setAttachments(colorBlendAttachments);

    creator.setShaderStages({ m_vertexShader->getCreateInfo(), m_fragmentShader->getCreateInfo() });

    m_pipeline = util::make_ptr<GraphicsPipeline>(
      creator.finishCreate(getOwningDevice(), m_layout, *m_pass, 0, true)
      );
  }


  void GeometryStep::setupRenderPass(std::vector<util::Ref<Image>> const& images) {
    static constexpr uint32_t NUM_GBUFFER_IMAGES = 3;
    assert(images.size() == NUM_GBUFFER_IMAGES + 1);

    m_pass = util::make_ptr<RenderPass>(getOwningDevice());
    m_pass->reserveAttachments(1);
    m_pass->reserveSubpasses(1);
    m_pass->reserveAttachmentRefs(1);
    m_pass->reserveSubpassDependencies(1);

    for (uint32_t i = 0; i < NUM_GBUFFER_IMAGES; ++i) {
      m_pass->addAttachment(images[i].get().getAttachmentDesc(VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
      m_pass->addAttachmentRef(i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, RenderPass::arfColor);
    }

    m_pass->addAttachment(images.back().get().getAttachmentDesc(VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));
    m_pass->addAttachmentRef(NUM_GBUFFER_IMAGES,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      RenderPass::arfDepthStencil);
    m_pass->finishSubpass();

    m_pass->addSubpassDependency({
                                   VK_SUBPASS_EXTERNAL,
                                   0,
                                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                   VK_ACCESS_MEMORY_READ_BIT,
                                   VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                   VK_DEPENDENCY_BY_REGION_BIT
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
}