// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Vertex.h
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

#ifndef DW_VERTEX_H
#define DW_VERTEX_H

#include "MyMath.h"

#include <vector>

struct VkVertexInputBindingDescription;
struct VkVertexInputAttributeDescription;

namespace dw {
  struct Vertex {
    glm::vec3 pos   { 0.f, 0.f, 0.f };
    glm::vec3 normal{ 0.f, 0.f, 1.f };
    glm::vec3 color { 1.f, 1.f, 1.f };

    static constexpr unsigned NUM_BINDING_ATTRIBUTES = 3;

    static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription> GetBindingAttributes();
  };
}

#endif
