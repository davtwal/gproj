// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : ImageLoader.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 10m 01d
// * Last Altered: 2019y 10m 01d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdexcept>
#include <set>
#include <algorithm>
#include <cassert>

namespace dw {

  class STBILoadedImageManager {
  public:
    static stbi_uc* Load(std::string const& name);
    static void Free(stbi_uc* data);

    static void FreeAll();

  private:
    static std::set<stbi_uc*> s_loadedImages;
  };

  std::set<stbi_uc*> STBILoadedImageManager::s_loadedImages;

  stbi_uc* STBILoadedImageManager::Load(std::string const& name) {
    stbi_set_flip_vertically_on_load(true);

    int width = 0, height = 0, channels = 0;
    stbi_uc* chars = stbi_load(name.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    assert(channels == 4);
    if (!chars)
      throw std::runtime_error("could not load image " + name);

    s_loadedImages.insert(chars);
  }

  void STBILoadedImageManager::Free(stbi_uc* image) {
    if (image) {
      stbi_image_free(image);
      s_loadedImages.erase(image);
    }
  }

  void STBILoadedImageManager::FreeAll() {
    for(auto& image : s_loadedImages) {
      stbi_image_free(image);
    }

    s_loadedImages.clear();
  }

  // IMAGE CREATION
  DependentImage Image::Load(LogicalDevice& device, std::string const& filename, uint32_t maxMipLevels) {
    DependentImage ret(device);

    stbi_uc* img = STBILoadedImageManager::Load(filename);

    //ret.initImage()

    return ret;
  }

}
