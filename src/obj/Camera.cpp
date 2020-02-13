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

#include "obj/Camera.h"

namespace dw {
  Camera::Camera() {
    setFOVDeg(m_fov);
    getView();
    getProj();
  }

  glm::vec3 Camera::getRightDir() const {
    return glm::normalize(glm::cross(UP_DIR, m_viewDir));
  }

  glm::vec3 Camera::getUpDir() const {
    return glm::normalize(glm::cross(m_viewDir, getRightDir()));
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
      m_proj = glm::perspective(m_fov, m_aspect, m_nearDist, m_farDist);
    }

    return m_proj;
  }

  glm::vec3 const& Camera::getEyePos() const {
    return m_eyePos;
  }

  glm::vec3 const& Camera::getViewDir() const {
    return m_viewDir;
  }

  float Camera::getAspect() const {
    return m_aspect;
  }

  float Camera::getFOV() const {
    return m_fov;
  }

  float Camera::getFOVDeg() const {
    return glm::degrees(m_fov);
  }

  float Camera::getNearDist() const {
    return m_nearDist;
  }

  float Camera::getFarDist() const {
    return m_farDist;
  }

  Camera& Camera::setEyePos(glm::vec3 const& pos) {
    m_eyePos = pos;
    m_viewDirty = true;
    return *this;
  }

  Camera& Camera::setFarDepth(float far) {
    m_farDist = far;
    m_projDirty = true;
    return *this;
  }

  Camera& Camera::setNearDepth(float near) {
    m_nearDist = near;
    m_projDirty = true;
    return *this;
  }

  Camera& Camera::setLookAt(glm::vec3 const& look) {
    return setViewDir(glm::normalize(look - m_eyePos));
  }

  Camera& Camera::setViewDir(glm::vec3 const& dir) {
    m_viewDir = dir;
    m_viewDirty = true;
    return *this;
  }

  //Camera& Camera::setDimensions(float width, float height) {
  //  m_width = width;
  //  m_height = height;
  //  m_projDirty = true;
  //  return *this;
  //}


  Camera& Camera::setFOVDeg(float degrees) {
    return setFOV(glm::radians(degrees));
  }

  Camera& Camera::setAspect(float aspect) {
    m_aspect = aspect;
    m_projDirty = true;
    return *this;
  }

  Camera& Camera::setFOV(float radians) {
    m_fov = radians;
    m_projDirty = true;
    return *this;
  }


}