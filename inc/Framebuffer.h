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

namespace dw {
  class RenderPass;
  class ImageView;
  CREATE_DEVICE_DEPENDENT(Framebuffer)
  public:
    Framebuffer(LogicalDevice& device, const RenderPass& pass, std::vector<VkImageView> const& attachments, VkExtent3D sizes);
    ~Framebuffer();

    operator VkFramebuffer() const;

  private:
    VkFramebuffer m_framebuffer{ nullptr };
  };

}

#endif
