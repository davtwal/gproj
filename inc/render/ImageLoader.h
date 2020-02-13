// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : ImageLoader.h
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

#ifndef DW_IMAGE_LOADER_H
#define DW_IMAGE_LOADER_H

#include <string>

namespace dw {
  class LoadedImage {

    unsigned char* m_bytes;
  };

  class ImageLoader {
    void loadImage(std::string const& name);


  };
}

#endif
