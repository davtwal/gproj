// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Light.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 10m 04d
// * Last Altered: 2019y 10m 04d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#ifndef DW_LIGHT_H
#define DW_LIGHT_H

#include "MyMath.h"

namespace dw {
  struct LightUBO {
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 dir;
    alignas(16) glm::vec3 color;
    alignas(04) float radius;
    alignas(04) int type; // 0, 1, 2
  };

  class Light {
  public:
    enum class Type {
      Point,        // does not use direction
      Spot,         // 
      Directional   // does not use position/radius
    };

    glm::vec3 m_position;   // world coordinates
    glm::vec3 m_direction;  // should be unit vector
    glm::vec3 m_color;      // should be 0..1
    float m_localRadius;    // world coordinates
    Type m_type{ Type::Point };

    LightUBO getAsUBO() const {
      return { m_position, m_direction, m_color, m_localRadius, (int)m_type };
    }
    // todo: attentuation
  };
}

#endif
