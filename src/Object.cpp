// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Object.cpp
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

#include "Object.h"

namespace dw {
  Object::Object(util::Ref<Mesh> const& mesh)
    : m_mesh(mesh){
    getTransform();
  }

  glm::mat4 const& Object::getTransform() {
    if(m_isDirty) {
      m_transform = glm::translate(glm::identity<glm::mat4>(), m_position) * glm::mat4_cast(m_rotation) * glm::scale(glm::identity<glm::mat4>(), m_scale);
      m_isDirty = false;
    }

    return m_transform;
  }

  glm::vec3 const& Object::getPosition() const {
    return m_position;
  }

  glm::quat const& Object::getRotation() const {
    return m_rotation;
  }

  glm::vec3 const& Object::getScale() const {
    return m_scale;
  }

  Object& Object::setPosition(glm::vec3 const& pos) {
    m_position = pos;
    m_isDirty = true;
    return *this;
  }

  Object& Object::setRotation(glm::quat const& rot) {
    m_rotation = rot;
    m_isDirty = true;
    return *this;
  }

  Object& Object::setScale(glm::vec3 const& scale) {
    m_scale = scale;
    m_isDirty = true;
    return *this;
  }

  void Object::callBehavior(float curTime, float dt) {
    if(m_behavior)
      m_behavior(*this, curTime, dt);
  }

}