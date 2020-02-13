// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Render_ShadowMapStep.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 10m 15d
// * Last Altered: 2019y 11m 15d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "render/Renderer.h"
#include "render/RenderSteps.h"
#include "render/Framebuffer.h"
#include "render/CommandBuffer.h"
#include "render/Shader.h"
#include "render/Image.h"

#include <stdexcept>
#include <array>

namespace dw {
  ShadowMapStep::ShadowMapStep(LogicalDevice& device, CommandPool& pool)
    : RenderStep(device),
      m_cmdBuff(pool.allocateCommandBuffer()) {
  }

  ShadowMapStep::ShadowMapStep(ShadowMapStep&& o) noexcept
    : RenderStep(std::move(o)),
      m_vertexShader(std::move(o.m_vertexShader)),
      m_fragmentShader(std::move(o.m_fragmentShader)),
      m_cmdBuff(o.m_cmdBuff),
      m_descriptorSet(o.m_descriptorSet) {
    o.m_vertexShader   = nullptr;
    o.m_fragmentShader = nullptr;
    o.m_descriptorSet  = nullptr;
  }

  void ShadowMapStep::setupPipelineLayout(VkPipelineLayout layout) {
    if (!layout) {
      std::array<VkPushConstantRange, 2> push_constant_ranges = {
        {
          {
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(int)
          },
          {
            VK_SHADER_STAGE_FRAGMENT_BIT,
            sizeof(int),
            sizeof(float) * 2
          }
        }
      };

      VkPipelineLayoutCreateInfo pipeLayoutInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        1,
        &m_descSetLayout,
        2,
        push_constant_ranges.data()
      };

      vkCreatePipelineLayout(getOwningDevice(), &pipeLayoutInfo, nullptr, &m_layout);
      assert(m_layout);
    }
    else
      m_layout = layout;
  }


  void ShadowMapStep::setupRenderPass(std::vector<util::Ref<Image>> const& images) {
    // TODO: Not sure if possible but maybe make this entire pass a subpass in geometry step? idk

    // we dont want to pass any images to this renderpass setup, as we dont use any.
    // instead, we force the attachment descriptions.
    assert(images.size() == 0);
    m_pass = util::make_ptr<RenderPass>(getOwningDevice());

    m_pass->addAttachment({
                            0,
                            VK_FORMAT_R32G32B32A32_SFLOAT,
                            VK_SAMPLE_COUNT_1_BIT,
                            VK_ATTACHMENT_LOAD_OP_CLEAR,
                            VK_ATTACHMENT_STORE_OP_STORE,
                            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                            VK_ATTACHMENT_STORE_OP_DONT_CARE,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                          });

    m_pass->addAttachment({
                            0,
                            VK_FORMAT_D24_UNORM_S8_UINT,
                            VK_SAMPLE_COUNT_1_BIT,
                            VK_ATTACHMENT_LOAD_OP_CLEAR,
                            VK_ATTACHMENT_STORE_OP_STORE,
                            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                            VK_ATTACHMENT_STORE_OP_DONT_CARE,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                          });

    m_pass->addAttachmentRef(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, RenderPass::arfColor);
    m_pass->addAttachmentRef(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, RenderPass::arfDepthStencil);

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

  void ShadowMapStep::setupPipeline(VkExtent2D extent) {
    GraphicsPipelineCreator creator;
    creator.setVertexType(Vertex::GetBindingDescriptions(), Vertex::GetBindingAttributes());

    creator.setViewport({
                          0,
                          0,//static_cast<float>(extent.height),
                          static_cast<float>(extent.width),
                          static_cast<float>(extent.height),
                          0,
                          1.f
                        });

    creator.setScissor({
                         {0, 0},
                         extent
                       });

    creator.setFrontFace(VK_FRONT_FACE_CLOCKWISE);
    creator.setDepthTesting(true);

    VkPipelineColorBlendAttachmentState colorAttachmentInfo = {
      VK_FALSE, // This is disabled because transparency is not currently allowed during the geometry pass.
      VK_BLEND_FACTOR_ONE,
      VK_BLEND_FACTOR_ZERO,
      VK_BLEND_OP_ADD,
      VK_BLEND_FACTOR_ONE,
      VK_BLEND_FACTOR_ZERO,
      VK_BLEND_OP_ADD,
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    creator.setAttachments({colorAttachmentInfo});

    creator.setShaderStages({m_vertexShader->getCreateInfo(), m_fragmentShader->getCreateInfo()});

    m_pipeline = util::make_ptr<GraphicsPipeline>(
                                                  creator.finishCreate(getOwningDevice(), m_layout, *m_pass, 0, true)
                                                 );
  }

  void ShadowMapStep::setupShaders() {
    // TODO: instead of loading a shader multiple times, have a shader manager that i can ask for a shader
    m_vertexShader = util::make_ptr<Shader<ShaderStage::Vertex>>(
                                                                 ShaderModule::Load(getOwningDevice(),
                                                                                    "shadow_map_vert.spv"));
    m_fragmentShader = util::make_ptr<Shader<ShaderStage::Fragment>>(
                                                                     ShaderModule::Load(getOwningDevice(),
                                                                                        "shadow_map_frag.spv"));
  }

  void ShadowMapStep::setupDescriptors() {
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
      { // lights
        0,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        1,
        VK_SHADER_STAGE_VERTEX_BIT,
        nullptr
      },
      { // model
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

  CommandBuffer& ShadowMapStep::getCommandBuffer() const {
    return m_cmdBuff;
  }

  void ShadowMapStep::writeCmdBuff(std::vector<Renderer::ShadowMappedLight> const& lights,
                                   Scene::ObjContainer const&                      scene,
                                   uint32_t                                        alignment,
                                   VkRect2D                                        renderArea) const {

    if (!lights.empty()) {
      auto& cmdBuff = m_cmdBuff.get();

      cmdBuff.start(false);


      // for each shadow mapped light
      for (uint32_t i = 0; i < lights.size(); ++i) {
        vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
        auto& light = lights.at(i);
        assert(light.m_depthBuffer);

        if (renderArea.extent.width == 0) {
          renderArea.extent = light.m_depthBuffer->getExtent();
        }

        std::array<VkClearValue, 2> clearValues{};
        clearValues.front().color       = {{0}};
        clearValues.back().depthStencil = {1.f, 0};

        // render the scene
        VkRenderPassBeginInfo beginInfo = {
          VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
          nullptr,
          *m_pass,
          *light.m_depthBuffer,
          renderArea,
          static_cast<uint32_t>(clearValues.size()),
          clearValues.data()
        };

        float depths[2] = {light.m_light.getNear(), light.m_light.getFar()};
        vkCmdPushConstants(cmdBuff, m_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(int), &i);
        vkCmdPushConstants(cmdBuff, m_layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(int), sizeof(float) * 2, depths);
        // TODO: have the rendering of the scene be a secondary command buffer?
        renderScene(cmdBuff, beginInfo, scene, alignment, m_layout, m_descriptorSet);
      }

      cmdBuff.end();
    }
  }

  void ShadowMapStep::updateDescriptorSets(Buffer& modelUBO, Buffer& lightsUBO) const {
    VkDescriptorBufferInfo modelUBOinfo = modelUBO.getDescriptorInfo();
    modelUBOinfo.range                  = sizeof(ObjectUniform);

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
        &lightsUBO.getDescriptorInfo(),
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
}
