// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Camera.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 09m 25d
// * Last Altered: 2019y 09m 25d
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

#ifndef DW_CAMERA_H
#define DW_CAMERA_H

#include "MyMath.h"

namespace dw {
  class Camera {
  public:
    Camera();

    static inline const glm::vec3 UP_DIR = { 0, 0, 1.f };

    glm::mat4 const& getView();
    glm::mat4 const& getProj();

    //Camera& setDimensions(float width, float height);
    Camera& setNearDepth(float near);
    Camera& setFarDepth(float far);
    Camera& setEyePos(glm::vec3 const& pos);
    Camera& setViewDir(glm::vec3 const& dir);
    Camera& setFOV(float radians);
    Camera& setFOVDeg(float degrees);

    // aspect = width / height;
    Camera& setAspect(float aspect);

    // changes the view direction to look at a specificpoint
    Camera& setLookAt(glm::vec3 const& look);

  private:

    //float m_width = 10;
    //float m_height = 10;
    float m_aspect = 1;
    float m_fov = 45.f;
    float m_nearDist = 0.1f;
    float m_farDist = 100.f;

    glm::vec3 m_eyePos {0, 0, 0};
    glm::vec3 m_viewDir {0, 0, -1};
    glm::mat4 m_view{1.f};
    glm::mat4 m_proj{1.f};
    bool m_viewDirty{ true };
    bool m_projDirty{ true };
  };
}

#endif
