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
    : m_device(device), m_extent(sizes) {

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

  Framebuffer::Framebuffer(LogicalDevice& device, VkExtent3D extent)
    : m_device(device), m_extent(extent) {
  }


  Framebuffer::Framebuffer(Framebuffer&& o) noexcept
    : m_device(o.m_device),
      m_framebuffer(o.m_framebuffer),
      m_images(std::move(o.m_images)),
      m_views(std::move(o.m_views))
  {
    o.m_framebuffer = nullptr;
    o.m_images.clear();
    o.m_views.clear();
  }

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

  VkExtent2D Framebuffer::getExtent() const {
    return { m_extent.width, m_extent.height };
  }

  std::vector<ImageView> const& Framebuffer::getImageViews() const {
    return m_views;
  }

  std::vector<DependentImage> const& Framebuffer::getImages() const {
    return m_images;
  }

  std::vector<ImageView>& Framebuffer::getImageViews() {
    return m_views;
  }

  std::vector<DependentImage>& Framebuffer::getImages() {
    return m_images;
  }

  void Framebuffer::finalize(const RenderPass& pass) {
    std::vector<VkImageView> imageViews{ m_views.begin(), m_views.end() };

    VkFramebufferCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      nullptr,
      0,
      pass,
      static_cast<uint32_t>(m_views.size()),
      imageViews.data(),
      m_extent.width,
      m_extent.height,
      m_extent.depth
    };

    if (vkCreateFramebuffer(m_device, &createInfo, nullptr, &m_framebuffer) != VK_SUCCESS)
      throw std::runtime_error("Could not create framebuffer");
  }

}
