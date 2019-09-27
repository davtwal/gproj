// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : Framebuffer.cpp
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

#include "Framebuffer.h"
#include "RenderPass.h"
#include "Image.h"
#include <stdexcept>
#include <cassert>

namespace dw {
  Framebuffer::Framebuffer(LogicalDevice& device, const RenderPass& pass, std::vector<VkImageView> const& attachments, VkExtent3D sizes)
    : m_device(device) {

    VkFramebufferCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      nullptr,
      0,
      pass,
      static_cast<uint32_t>(attachments.size()),
      attachments.data(),
      sizes.width,
      sizes.height,
      sizes.depth
    };

    if (vkCreateFramebuffer(m_device, &createInfo, nullptr, &m_framebuffer) != VK_SUCCESS)
      throw std::runtime_error("Could not create framebuffer");
  }

  Framebuffer::Framebuffer(Framebuffer&& o) noexcept
    : m_device(o.m_device),
      m_framebuffer(o.m_framebuffer) {
    o.m_framebuffer = nullptr;
  }

  /*Framebuffer& Framebuffer::operator=(Framebuffer&& o) noexcept {
    assert(o.m_device == m_device);

    m_framebuffer = o.m_framebuffer;
    o.m_framebuffer = nullptr;

    return *this;
  }*/


  Framebuffer::~Framebuffer() {
    if (m_framebuffer)
      vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
  }

  Framebuffer::operator VkFramebuffer() const {
    return m_framebuffer;
  }

  Framebuffer::operator VkFramebuffer() {
    return m_framebuffer;
  }
}
