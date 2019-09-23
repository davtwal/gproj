// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : Swapchain.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 16d
// * Last Altered: 2019y 09m 16d
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

#ifndef DW_VK_SWAPCHAIN_H
#define DW_VK_SWAPCHAIN_H

#include "Surface.h"
#include "LogicalDevice.h"
#include "Image.h"
#include "Utils.h"

namespace dw {
  class RenderPass;
  class Framebuffer;

  CREATE_DEVICE_DEPENDENT(Swapchain)
  public:
    Swapchain(LogicalDevice& device, Surface& surface, util::Ref<Queue> q);
    ~Swapchain();

    void restart();

    uint32_t getNextImageIndex();

    // this semaphore is signaled when the image returned from
    // getNextImage() and getNextImageIndex() are done being
    // used and can be rendered to.
    // if this semaphore is signaled then that means
    // we can draw to the image.
    NO_DISCARD VkSemaphore const& getNextImageSemaphore() const;
    NO_DISCARD VkSemaphore const& getImageRenderReadySemaphore() const;

    NO_DISCARD Image const& getNextImage() const;
    NO_DISCARD ImageView const& getNextImageView() const;

    NO_DISCARD VkAttachmentDescription getImageAttachmentDesc() const;

    NO_DISCARD bool isPresentReady() const;

    NO_DISCARD size_t getNumImages() const;

    NO_DISCARD VkExtent2D getImageSize() const;
    NO_DISCARD VkFormat getImageFormat() const;

    void present();
    void present(Queue const& q);

    void createFramebuffers(RenderPass const& renderPass);

    NO_DISCARD std::vector<IndependentImage> const& getImages() const;
    NO_DISCARD std::vector<ImageView> const& getViews() const;
    NO_DISCARD std::vector<Framebuffer> const& getFrameBuffers() const;

  private:
    void createSemaphores();
    void cleanupSemaphores();
    void queryImages();
    void createViews();

    Surface& m_surface;
    util::Ref<Queue> m_queue;
    VkSwapchainKHR m_swapchain{ nullptr };
    VkFormat m_imageFormat;

    std::vector<IndependentImage> m_images;
    std::vector<ImageView> m_views;
    std::vector<Framebuffer> m_framebuffers;
    VkSemaphore m_nextImageSemaphore{ nullptr };
    VkSemaphore m_imageRenderReady{ nullptr };  // after a command buffer has been executed
    uint32_t m_nextImage{ 0 };
  };
}

#endif

