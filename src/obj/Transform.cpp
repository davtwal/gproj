// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Transform.cpp
// * Copyright (C) DigiPen Institute of Technology 2020
// * 
// * Created     : 2020y 02m 12d
// * Last Altered: 2020y 02m 12d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "obj/Transform.h"

namespace dw::obj {
  glm::mat4 const& Transform::getMatrix() {
    if (m_dirty) {
      glm::mat4 ident = glm::identity<glm::mat4>();
      m_matrix = glm::translate(ident, m_position) * glm::mat4_cast(m_rotation) * glm::scale(ident, m_scale);

      m_dirty = false;
    }
    return m_matrix;
  }

  glm::vec3 const& Transform::getPosition() const {
    return m_position;
  }

  glm::vec3 const& Transform::getScale() const {
    return m_scale;
  }

  glm::quat const& Transform::getRotation() const {
    return m_rotation;
  }

  Transform& Transform::setPosition(glm::vec3 const& newPos) {
    m_position = newPos;
    m_dirty = true;
    return *this;
  }

  Transform& Transform::addPosition(glm::vec3 const& plusPos) {
    m_position += plusPos;
    m_dirty = true;
    return *this;
  }

  Transform& Transform::setScale(glm::vec3 const& newScale) {
    m_scale = newScale;
    m_dirty = true;
    return *this;
  }

  Transform& Transform::addScale(glm::vec3 const& plusScale) {
    m_scale += plusScale;
    m_dirty = true;
    return *this;
  }

  Transform& Transform::setRotation(glm::quat const& newRot) {
    m_rotation = newRot;
    m_dirty = true;
    return *this;
  }

  Transform& Transform::addRotation(glm::quat const& plusRot) {
    m_rotation += plusRot;
    m_dirty = true;
    return *this;
  }
}