// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Light.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 10m 15d
// * Last Altered: 2019y 10m 15d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "Light.h"

namespace dw {
  // GETTERS
  glm::vec3 const& Light::getPosition() const {
    return m_position;
  }

  glm::vec3 const& Light::getDirection() const {
    return m_direction;
  }

  glm::vec3 const& Light::getColor() const {
    return m_color;
  }

  float Light::getLocalRadius() const {
    return m_localRadius;
  }

  glm::vec3 const& Light::getAttenuation() const {
    return m_attenuation;
  }

  Light::Type Light::getType() const {
    return m_type;
  }

  // SETTERS
  Light& Light::setPosition(glm::vec3 const& pos) {
    m_position = pos;
    return *this;
  }

  Light& Light::setDirection(glm::vec3 const& dir) {
    m_direction = dir;
    return *this;
  }

  Light& Light::setColor(glm::vec3 const& color) {
    m_color = color;
    return *this;
  }

  Light& Light::setAttenuation(glm::vec3 const& atten) {
    m_attenuation = atten;
    return *this;
  }

  Light& Light::setLocalRadius(float rad) {
    m_localRadius = rad;
    return *this;
  }

  Light& Light::setType(Type t) {
    m_type = t;
    return *this;
  }

  // SHADOWED
  glm::mat4 const& ShadowedLight::getView() {
    if(m_isViewDirty) {
      m_view = glm::lookAt(m_position, m_position + m_direction, UP_DIR);

      m_isViewDirty = false;
    }

    return m_view;
  }

  glm::mat4 const& ShadowedLight::getProj() {
    if(m_isProjDirty) {
      m_proj = glm::perspective(glm::radians(45.f), 1.f, 0.5f, 50.f);

      m_isProjDirty = false;
    }

    return m_proj;
  }

  // setters
  Light& ShadowedLight::setPosition(glm::vec3 const& pos) {
    m_position = pos;
    m_isViewDirty = true;
    return *this;
  }

  Light& ShadowedLight::setDirection(glm::vec3 const& dir) {
    m_direction = dir;
    m_isViewDirty = true;
    return *this;
  }


}