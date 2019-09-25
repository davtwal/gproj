// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Vertex.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 24d
// * Last Altered: 2019y 09m 24d
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

#include "Vertex.h"

#include <vulkan/vulkan.h>

namespace dw {
  std::vector<VkVertexInputBindingDescription> Vertex::GetBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> ret;
    // BINDING, SIZE, RATE
    ret.push_back({0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX});
    return ret;
  }

  std::vector<VkVertexInputAttributeDescription> Vertex::GetBindingAttributes() {
    std::vector<VkVertexInputAttributeDescription> ret;
    ret.reserve(NUM_BINDING_ATTRIBUTES);
      // LOC, BINDING, FORMAT, OFFSET
    ret.push_back({ 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 });
    ret.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) });
    return ret;
  }
}