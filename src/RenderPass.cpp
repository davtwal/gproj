// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : RenderPass.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 20d
// * Last Altered: 2019y 09m 20d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * Description :
// *
// *
// *
// *
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#include "RenderPass.h"
#include "Trace.h"
#include <cassert>

namespace dw {
  RenderPass::RenderPass(LogicalDevice& device)
    : m_device(device) {

    startConstruction();
  }

  void RenderPass::addAttachment(VkAttachmentDescription const& desc) {
    if(m_conInf) {
      m_conInf->attachments.push_back(desc);
    }
  }

  void RenderPass::addInputRef(uint32_t index, VkImageLayout layout) {
    if(m_conInf) {
      m_conInf->sbc[m_conInf->subpasses.size()].inputRefs.push_back({ index, layout });
    }
  }

  void RenderPass::addPreserveRef(uint32_t index) {
    if(m_conInf) {
      m_conInf->sbc[m_conInf->subpasses.size()].preserveRefs.push_back(index);
    }
  }

  void RenderPass::addAttachmentRef(uint32_t index, VkImageLayout layout, AttachmentRefFlags refType) {
    if (m_conInf) {
      size_t i = m_conInf->subpasses.size();
      auto& sbc = m_conInf->sbc[i];

      if (refType & arfColor) {
        sbc.colorRefs.push_back({ index, layout });

        if (refType & arfResolve) {
          if (sbc.resolveRefs.empty()) {
            sbc.resolveRefs.reserve(sbc.colorRefs.capacity());
            for (auto& r : sbc.colorRefs) {
              sbc.resolveRefs.push_back({ VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED });
            }
          }

          sbc.resolveRefs.push_back({ index, layout });
        }
      }

      else if (refType & arfDepthStencil) {
        if (sbc.depthStencilRefs.empty()) {
          sbc.depthStencilRefs.reserve(sbc.colorRefs.capacity());
          for (auto& r : sbc.colorRefs) {
            sbc.depthStencilRefs.push_back({ VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED });
          }
        }

        sbc.depthStencilRefs.push_back({ index, layout });
      }

      else
        Trace::Warn << "Attempted to add an attachment ref that was not color, color/resolve, or depth stencil" << Trace::Stop;
    }
  }

  void RenderPass::addSubpassDependency(VkSubpassDependency const& dep) {
    assert(dep.srcSubpass <= dep.dstSubpass || dep.srcSubpass == VK_SUBPASS_EXTERNAL);
    if(m_conInf) {
      m_conInf->dependencies.push_back(dep);
    }
  }

  bool RenderPass::isConstructed() const {
    return m_conInf == nullptr;
  }

  void RenderPass::finishSubpass() {
    if(m_conInf) {
      size_t i = m_conInf->subpasses.size();
      auto& sbc = m_conInf->sbc[i];

      m_conInf->subpasses.push_back({
        0,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        static_cast<uint32_t>(sbc.inputRefs.size()),
        sbc.inputRefs.data(),
        static_cast<uint32_t>(sbc.colorRefs.size()),
        sbc.colorRefs.data(),
        sbc.resolveRefs.data(),
        sbc.depthStencilRefs.data(),
        static_cast<uint32_t>(sbc.preserveRefs.size()),
        sbc.preserveRefs.data()
      });

      m_conInf->sbc.push_back({});
    }
  }

  void RenderPass::finishRenderPass() {
    VkRenderPassCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(m_conInf->attachments.size()),
      m_conInf->attachments.data(),
      static_cast<uint32_t>(m_conInf->subpasses.size()),
      m_conInf->subpasses.data(),
      static_cast<uint32_t>(m_conInf->dependencies.size()),
      m_conInf->dependencies.data()
    };

    vkCreateRenderPass(m_device, &createInfo, nullptr, &m_pass);

    if (!m_pass)
      throw std::runtime_error("Could not create render pass");

    delete m_conInf;
    m_conInf = nullptr;
  }

  void RenderPass::destroy() {
    if(m_pass) {
      vkDestroyRenderPass(m_device, m_pass, nullptr);
      m_pass = nullptr;
    }
  }

  void RenderPass::startConstruction() {
    destroy();

    m_conInf = new RenderPassConstruction;
    m_conInf->sbc.resize(1);
  }

  void RenderPass::reserveAttachmentRefs(uint32_t num) {
    if (m_conInf) {
      if(num > m_conInf->sbc[m_conInf->subpasses.size()].colorRefs.capacity())
        m_conInf->sbc[m_conInf->subpasses.size()].colorRefs.reserve(num);
    }
  }

  void RenderPass::reserveAttachments(uint32_t num) {
    if(m_conInf) {
      if(num > m_conInf->attachments.capacity())
        m_conInf->attachments.reserve(num);
    }
  }

  void RenderPass::reserveSubpasses(uint32_t num) {
    if(m_conInf) {
      // num + 1 is used here because once finishSubpass is called, another empty construction
      // info is pushed onto the vector, and i don't want to reallocate.
      if (1LL + num > m_conInf->subpasses.capacity())
        m_conInf->subpasses.reserve(1LL + num);
    }
  }

  void RenderPass::reserveSubpassDependencies(uint32_t num) {
    if(m_conInf) {
      if (num > m_conInf->dependencies.capacity())
        m_conInf->dependencies.reserve(num);
    }
  }

  RenderPass::operator VkRenderPass() const {
    return m_pass;
  }
}
