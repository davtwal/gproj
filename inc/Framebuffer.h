// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : Framebuffer.h
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

#ifndef DW_FRAMEBUFFER_H
#define DW_FRAMEBUFFER_H

#include "LogicalDevice.h"
#include "MemoryAllocator.h"
#include "Image.h"

namespace dw {
  class RenderPass;
  class Image;
  class ImageView;
  CREATE_DEVICE_DEPENDENT(Framebuffer)
  public:
    // This constructor is used for INDEPENDENT IMAGES
    Framebuffer(LogicalDevice& device, const RenderPass& pass, std::vector<VkImageView> const& attachments, VkExtent3D sizes);

    // This constructor is used for DEPENDENT IMAGES
    Framebuffer(LogicalDevice& device, VkExtent3D extent);

    ~Framebuffer();

    operator VkFramebuffer() const;
    operator VkFramebuffer();

    // TODO make this function better
    template<typename... Args>
    void addImage(VkFlags viewAspect, Args&&... args) {
      m_images.emplace_back(m_device).initImage(std::move(args)...);
      MemoryAllocator allocator(m_device.getOwningPhysical());
      m_images.back().back(allocator, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      m_views.emplace_back(m_images.back().createView(viewAspect));
    }

    void finalize(const RenderPass& pass);

    //void resize(VkExtent2D extent);

    NO_DISCARD VkExtent2D getExtent() const;
    NO_DISCARD std::vector<DependentImage> const& getImages() const;
    NO_DISCARD std::vector<ImageView> const& getImageViews() const;
    NO_DISCARD std::vector<DependentImage>& getImages();
    NO_DISCARD std::vector<ImageView>& getImageViews();
    // the non-const need to exist or else vector copying to references
    // will break


  private:
    VkFramebuffer m_framebuffer{ nullptr };
    std::vector<DependentImage> m_images;
    std::vector<ImageView> m_views;
    VkExtent3D m_extent;
  };

}

#endif
