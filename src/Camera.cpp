// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Camera.cpp
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

#include "Camera.h"

namespace dw {
  Camera::Camera() {
    getView();
    getProj();
  }


  glm::mat4 const& Camera::getView() {
    if(m_viewDirty) {
      m_view = glm::lookAt(m_eyePos, m_eyePos + m_viewDir, UP_DIR);
      m_viewDirty = false;
    }

    return m_view;
  }

  glm::mat4 const& Camera::getProj() {
    if(m_projDirty) {
      float fovy = 2 * atan2f(m_height, 2 * m_nearDist);
      m_proj = glm::perspective(fovy, m_width / m_height, m_nearDist, m_farDist);
    }

    return m_proj;
  }


}