// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : RenderSteps.cpp
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

#include "Renderer.h"
#include "RenderSteps.h"
#include "Framebuffer.h"
#include "CommandBuffer.h"

#include <stdexcept>
#include <array>
#include "Shader.h"

namespace dw {
  /* GEOMETRY STEP
   *
   */
  void GeometryStep::writeCmdBuff(Framebuffer& fb, Renderer::SceneContainer const& scene, uint32_t alignment, VkRect2D renderArea) {
    // 1: deferred pass
    if(renderArea.extent.width == 0) {
      renderArea.extent = fb.getExtent();
    }

    if (!scene.empty()) {
      std::array<VkClearValue, 4> clearValues{};
      clearValues[0].color = { {0, 0, 0, 0} };
      clearValues[1].color = { {0} };
      clearValues[2].color = { {0} };
      clearValues[3].depthStencil = { 1.f, 0 };

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
      vkCmdBeginRenderPass(commandBuff, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

      Mesh* curMesh = nullptr;

      for (uint32_t j = 0; j < scene.size(); ++j) {
        auto& obj = scene.at(j);

        if (!curMesh || !(obj.get().m_mesh.get() == *curMesh)) {
          curMesh = &obj.get().m_mesh.get();

          const VkBuffer& buff = curMesh->getVertexBuffer();
          const VkDeviceSize offset = 0;
          vkCmdBindVertexBuffers(commandBuff, 0, 1, &buff, &offset);
          vkCmdBindIndexBuffer(commandBuff, curMesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
        }

        // One dynamic offset per dynamic descriptor to offset into the ubo containing all model matrices
        uint32_t dynamicOffset = j * alignment;
        vkCmdBindDescriptorSets(commandBuff,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          m_layout,
          0,
          1,
          &m_descriptorSet,
          1,
          &dynamicOffset);

        vkCmdDrawIndexed(commandBuff, curMesh->getNumIndices(), 1, 0, 0, 0);
      }

      vkCmdEndRenderPass(commandBuff);
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

  void GeometryStep::updateDescriptorSets(Buffer& modelUBO, Buffer& cameraUBO) {
    // Descriptor sets are automatically freed once the pool is freed.
    // They can be individually freed if the pool was created with
    // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT sets
    VkDescriptorBufferInfo modelUBOinfo = modelUBO.getDescriptorInfo();
    modelUBOinfo.range = sizeof(ObjectUniform);


    std::vector<VkWriteDescriptorSet> descriptorWrites = { {
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
    m_vertexShader = util::make_ptr<Shader<ShaderStage::Vertex>>(ShaderModule::Load(getOwningDevice(),
      "fromBuffer_transform_vert.spv"));
    m_fragmentShader = util::make_ptr<Shader<ShaderStage::Fragment>>(ShaderModule::Load(getOwningDevice(),
      "triangle_frag.spv"));
  }

  void GeometryStep::setupPipelineLayout() {
    VkPipelineLayoutCreateInfo pipeLayoutInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      1,
      &m_descSetLayout,
      0,
      nullptr
    };

    if (!m_descSetLayout) {
      pipeLayoutInfo.pSetLayouts = nullptr;
      pipeLayoutInfo.setLayoutCount = 0;
    }

    vkCreatePipelineLayout(getOwningDevice(), &pipeLayoutInfo, nullptr, &m_layout);
    assert(m_layout);
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

    static constexpr uint32_t GBUFFER_IMAGE_COUNT = 3;
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

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments{GBUFFER_IMAGE_COUNT, colorAttachmentInfo};

    creator.setShaderStages({ m_vertexShader->getCreateInfo(), m_fragmentShader->getCreateInfo() });

    m_pipeline = util::make_ptr<GraphicsPipeline>(
      creator.finishCreate(getOwningDevice(), m_layout, *m_pass, 0, true)
    );
  }


  void GeometryStep::setupRenderPass(uint32_t gbufferImageCount) {
    m_pass = util::make_ptr<RenderPass>(getOwningDevice());
    m_pass->reserveAttachments(1);
    m_pass->reserveSubpasses(1);
    m_pass->reserveAttachmentRefs(1);
    m_pass->reserveSubpassDependencies(1);

    for (uint32_t i = 0; i < m_gbufferImages.size(); ++i) {
      m_pass->addAttachment(m_gbufferImages[i].getAttachmentDesc(VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
      m_pass->addAttachmentRef(i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, RenderPass::arfColor);
    }

    m_pass->addAttachment(m_depthStencilImage->getAttachmentDesc(VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));
    m_pass->addAttachmentRef(m_gbufferImages.size(),
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

  /* FINAL PASS
   *
   */
  void FinalStep::setupDescriptors() {
    uint32_t numSampledImages = m_gbufferImages.size();

    std::vector<VkDescriptorSetLayoutBinding> finalBindings;
    finalBindings.
      resize(numSampledImages + 2); // one sampler per gbuffer image + one view eye/view dir UBO + one for light UBO

    finalBindings.front() = {
      0,
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      1,
      VK_SHADER_STAGE_FRAGMENT_BIT,
      nullptr
    };

    for (uint32_t i = 1; i < finalBindings.size() - 1; ++i) {
      finalBindings[i] = {
        i,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        1,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        nullptr
      };
    }

    finalBindings.back() = {
      4,
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
    VkDescriptorSetAllocateInfo descSetAllocInfo = {
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
    m_vertexShader = util::make_ptr<Shader<ShaderStage::Vertex>>(ShaderModule::Load(getOwningDevice(), "fsq_vert.spv"));
    m_fragmentShader = util::make_ptr<Shader<ShaderStage::Fragment>>(ShaderModule::Load(getOwningDevice(), "fsq_frag.spv"));
  }

}
