// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Swapchain.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 09m 23d
// * Last Altered: 2019y 09m 23d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "Swapchain.h"
#include "Image.h"
#include "Queue.h"
#include "Framebuffer.h"

#include <stdexcept>

namespace dw {
  Swapchain::Swapchain(LogicalDevice& device, Surface& surface, util::Ref<Queue> q)
    : m_device(device),
      m_surface(surface),
      m_queue(q) {

    createSemaphores();
    restart();
  }

  void Swapchain::restart() {
    VkSurfaceFormatKHR format = m_surface.chooseFormat();
    VkPresentModeKHR   mode   = m_surface.choosePresentMode(false);
    VkExtent2D         extent = m_surface.chooseExtent();

    m_imageFormat = format.format;

    VkSwapchainCreateInfoKHR createInfo = {
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      nullptr,
      0,
      m_surface,
      m_surface.getCapabilities().minImageCount + 1,
      format.format,
      format.colorSpace,
      extent,
      1,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr,
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      mode,
      VK_TRUE,
      m_swapchain
    };

    vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain);

    queryImages();
    createViews();
  }

  Swapchain::~Swapchain() {
    if (m_swapchain) {
      vkDestroySwapchainKHR(getOwningDevice(), m_swapchain, nullptr);
      m_swapchain = nullptr;
    }

    cleanupSemaphores();
  }

  void Swapchain::queryImages() {
    uint32_t numImages = 0;
    vkGetSwapchainImagesKHR(getOwningDevice(), m_swapchain, &numImages, nullptr);

    if (numImages) {
      std::vector<VkImage> images(numImages);
      vkGetSwapchainImagesKHR(getOwningDevice(), m_swapchain, &numImages, images.data());

      m_images.reserve(numImages);
      for (auto image : images)
        m_images.emplace_back(image, m_imageFormat, VK_IMAGE_VIEW_TYPE_2D, &getOwningDevice());
    }
  }

  void Swapchain::createViews() {
    m_views.reserve(m_images.size());
    for (auto& image : m_images) {
      m_views.emplace_back(image.createView());
    }
  }

  void Swapchain::createFramebuffers(RenderPass const& renderPass) {
    m_framebuffers.reserve(m_views.size());
    for (auto& view : m_views) {
      m_framebuffers.emplace_back(m_device,
                                  renderPass,
                                  std::vector<VkImageView>({view}),
                                  VkExtent3D{m_surface.getWidth(), m_surface.getHeight(), 1});
    }
  }

  void Swapchain::setFramebuffers(std::vector<Framebuffer>&& framebuffers) {
    m_framebuffers = std::move(framebuffers);
  }

  VkSemaphore const& Swapchain::getNextImageSemaphore() const {
    return m_nextImageSemaphore;
  }

  VkSemaphore const& Swapchain::getImageRenderReadySemaphore() const {
    return m_imageRenderReady;
  }


  uint32_t Swapchain::getNextImageIndex() {
    vkAcquireNextImageKHR(m_device, m_swapchain, 100, m_nextImageSemaphore, nullptr, &m_nextImage);
    return m_nextImage;
  }

  Image const& Swapchain::getNextImage() const {
    return m_images[m_nextImage];
  }

  VkAttachmentDescription Swapchain::getImageAttachmentDesc() const {
    VkAttachmentDescription desc = {
      0,
      m_imageFormat,
      VK_SAMPLE_COUNT_1_BIT,
      VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    return desc;
  }

  void Swapchain::present() {
    present(m_queue);
  }

  void Swapchain::present(Queue const& q) const {
    VkResult         out_result  = VK_SUCCESS;
    VkPresentInfoKHR presentInfo = {
      VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      nullptr,
      1,
      &getImageRenderReadySemaphore(),
      1,
      &m_swapchain,
      &m_nextImage,
      &out_result
    };

    if (vkQueuePresentKHR(q, &presentInfo) != VK_SUCCESS || out_result != VK_SUCCESS)
      throw std::runtime_error("Error while presenting");
  }

  void Swapchain::createSemaphores() {
    VkSemaphoreCreateInfo semCreate = {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      nullptr,
      0
    };

    vkCreateSemaphore(m_device, &semCreate, nullptr, &m_nextImageSemaphore);
    vkCreateSemaphore(m_device, &semCreate, nullptr, &m_imageRenderReady);
  }

  void Swapchain::cleanupSemaphores() {
    if (m_nextImageSemaphore) {
      vkDestroySemaphore(m_device, m_nextImageSemaphore, nullptr);
      m_nextImageSemaphore = nullptr;
    }

    if (m_imageRenderReady) {
      vkDestroySemaphore(m_device, m_imageRenderReady, nullptr);
      m_imageRenderReady = nullptr;
    }
  }

  size_t Swapchain::getNumImages() const {
    return m_images.size();
  }

  VkExtent2D Swapchain::getImageSize() const {
    return {m_surface.getWidth(), m_surface.getHeight()};
  }

  VkFormat Swapchain::getImageFormat() const {
    return m_imageFormat;
  }

  ImageView const& Swapchain::getNextImageView() const {
    return m_views[m_nextImage];
  }

  bool Swapchain::isPresentReady() const {
    return !m_images.empty()
           && m_views.size() == m_images.size()
           && m_framebuffers.size() == m_images.size()
           && m_nextImageSemaphore
           && m_imageRenderReady
           && m_imageFormat != VK_FORMAT_UNDEFINED
           && m_swapchain;
  }

  std::vector<Framebuffer> const& Swapchain::getFrameBuffers() const {
    return m_framebuffers;
  }

  std::vector<IndependentImage> const& Swapchain::getImages() const {
    return m_images;
  }

  std::vector<ImageView> const& Swapchain::getViews() const {
    return m_views;
  }

  std::vector<Framebuffer>& Swapchain::getFrameBuffers() {
    return m_framebuffers;
  }

  std::vector<IndependentImage>& Swapchain::getImages() {
    return m_images;
  }

  std::vector<ImageView>& Swapchain::getViews() {
    return m_views;
  }
}
