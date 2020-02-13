// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : RenderSteps.cpp
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

#include "render/Renderer.h"
#include "render/RenderSteps.h"
#include "render/Framebuffer.h"
#include "render/CommandBuffer.h"
#include "render/Shader.h"
#include "render/Image.h"

#include <stdexcept>
#include <array>

namespace dw {
  void RenderStep::renderScene(CommandBuffer&             commandBuff,
                               VkRenderPassBeginInfo&     beginInfo,
                               Scene::ObjContainer const& scene,
                               uint32_t                   alignment,
                               VkPipelineLayout           layout,
                               VkDescriptorSet            descriptorSet) {

    vkCmdBeginRenderPass(commandBuff, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

    Mesh* curMesh = nullptr;

    for (uint32_t j = 0; j < scene.size(); ++j) {
      auto& obj = scene.at(j);

      if (!curMesh || !(obj->m_mesh.get() == *curMesh)) {
        curMesh = &obj->m_mesh.get();

        const VkBuffer&    buff   = curMesh->getVertexBuffer();
        const VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuff, 0, 1, &buff, &offset);
        vkCmdBindIndexBuffer(commandBuff, curMesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
      }
      
      // One dynamic offset per dynamic descriptor to offset into the ubo containing all model matrices
      uint32_t dynamicOffset = j * alignment;
      vkCmdBindDescriptorSets(commandBuff,
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              layout,
                              0,
                              1,
                              &descriptorSet,
                              1,
                              &dynamicOffset);

      vkCmdDrawIndexed(commandBuff, curMesh->getNumIndices(), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuff);
  }

  RenderStep::RenderStep(LogicalDevice& device)
    : m_device(device) {
  }

  RenderStep::~RenderStep() {
    if (m_layout) {
      vkDestroyPipelineLayout(m_device, m_layout, nullptr);
      m_layout = nullptr;
    }

    if (m_descriptorPool) {
      vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
      m_descriptorPool = nullptr;
    }

    if (m_descSetLayout) {
      vkDestroyDescriptorSetLayout(m_device, m_descSetLayout, nullptr);
      m_descSetLayout = nullptr;
    }
  }

  RenderStep::RenderStep(RenderStep&& o) noexcept
    : m_device(o.m_device),
      m_layout(std::move(o.m_layout)),
      m_descSetLayout(std::move(o.m_descSetLayout)),
      m_descriptorPool(std::move(o.m_descriptorPool)),
      m_pipeline(std::move(o.m_pipeline)),
      m_pass(std::move(o.m_pass)) {
    o.m_layout         = nullptr;
    o.m_descSetLayout  = nullptr;
    o.m_descriptorPool = nullptr;
    o.m_pipeline.reset();
    o.m_pass.reset();
  }

  void RenderStep::setupPipelineLayout(VkPipelineLayout layout) {
    if (!layout) {
      VkPipelineLayoutCreateInfo pipeLayoutInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        1,
        &m_descSetLayout,
        0,
        nullptr
      };

      vkCreatePipelineLayout(getOwningDevice(), &pipeLayoutInfo, nullptr, &m_layout);
      assert(m_layout);
    }
    else
      m_layout = layout;
  }


  GraphicsPipeline& RenderStep::getPipeline() const {
    return *m_pipeline;
  }

  RenderPass& RenderStep::getRenderPass() const {
    return *m_pass;
  }

  VkPipelineLayout RenderStep::getLayout() const {
    return m_layout;
  }
}
