// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : ImageManager.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 10m 11d
// * Last Altered: 2019y 10m 11d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#ifndef DW_IMAGE_MANAGER_H
#define DW_IMAGE_MANAGER_H

#include "LogicalDevice.h"
#include "Image.h"
#include <unordered_map>

namespace dw {
  class ImageManager {
  public:
    using ImageKey = uint32_t;
    using ImageMap = std::unordered_map<ImageKey, DependentImage>;

  private:
    ImageMap m_map;

  };
}

#endif
