// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Camera.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 09m 26d
// * Last Altered: 2020y 02m 26d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "obj/Camera.h"
#include "obj/Object.h"

namespace dw::obj {
  const glm::vec3 Camera::DEFAULT_RIGHT_DIR{1.f, 0.f, 0.f};
  const glm::vec3 Camera::DEFAULT_WORLD_UP{0.f, 0.f, 1.f};
  const glm::vec3 Camera::DEFAULT_LOOK_DIR{0.f, 1.f, 0.f};

  ////////////////////
  // TRANSFORMATION
  glm::mat4 Camera::worldToCamera() {
    auto transfMtx = glm::identity<glm::mat4>();

    if (getParent()) {
      auto transform = getParent()->getTransform();
      if (transform) {
        transfMtx = translate(transfMtx, transform->getPosition()) * mat4_cast(transform->getRotation());
      }
    }

    if (m_viewDirty) {
      // I do not use getRight or getUp here as this takes less calculations
      // If a problem is occurring then that might help
      glm::vec3 dir   = getForward();
      glm::vec3 right = cross(getWorldUp(), dir);
      glm::vec3 up    = cross(dir, right);

      m_viewMtx = glm::lookAt(getWorldPos(), getWorldPos() + dir, up);

      /*m_viewMtx = glm::mat4(right.x, right.y,right.z,0.f,
                            up.x,up.y,up.z,0.f,
                            dir.x,dir.y, dir.z,0.f,
                            0.f,0.f,0.f, 1.f)
                  * translate(glm::identity<glm::mat4>(), -m_localPos);*/

      m_viewDirty = false;
    }

    return transfMtx * m_viewMtx;
  }

  glm::mat4 const& Camera::cameraToNDC() {
    if (m_projDirty) {
      m_projMtx   = glm::perspective(m_fov, m_aspect, m_near, m_far);
      m_projDirty = false;
    }

    return m_projMtx;
  }

  glm::mat4 Camera::cameraToWorld() {
    return inverse(worldToCamera());
  }

  glm::mat4 Camera::GetDefaultView() {
    return glm::mat4(DEFAULT_RIGHT_DIR.x,
                     DEFAULT_RIGHT_DIR.y,
                     DEFAULT_RIGHT_DIR.z,
                     0.f,
                     DEFAULT_WORLD_UP.x,
                     DEFAULT_WORLD_UP.y,
                     DEFAULT_WORLD_UP.z,
                     0.f,
                     DEFAULT_LOOK_DIR.x,
                     DEFAULT_LOOK_DIR.y,
                     DEFAULT_LOOK_DIR.z,
                     0.f,
                     0.f,
                     0.f,
                     0.f,
                     1.f);
  }

  glm::mat4 Camera::GetDefaultProj() {
    return glm::perspective(DEFAULT_FOV, DEFAULT_ASPECT, DEFAULT_NEAR, DEFAULT_FAR);
  }

  ////////////////////
  // SETTERS
  //////////
  // PROJECTION
  Camera& Camera::setAspect(float newAspect) {
    m_aspect    = newAspect;
    m_projDirty = true;
    return *this;
  }

  Camera& Camera::setFOV(float newFOV) {
    m_fov       = newFOV;
    m_projDirty = true;
    return *this;
  }

  Camera& Camera::setNear(float newNear) {
    m_near      = newNear;
    m_projDirty = true;
    return *this;
  }

  Camera& Camera::setFar(float newFar) {
    m_far       = newFar;
    m_projDirty = true;
    return *this;
  }

  //////////
  // VIEW
  Camera& Camera::setWorldUp(glm::vec3 const& newWorldUp) {
    m_worldUp   = normalize(newWorldUp);
    m_viewDirty = true;
    return *this;
  }

  Camera& Camera::setLocalPos(glm::vec3 const& newLocalPos) {
    m_localPos  = newLocalPos;
    m_viewDirty = true;
    return *this;
  }

  Camera& Camera::addLocalPos(glm::vec3 const& plusLocal) {
    m_localPos += plusLocal;
    m_viewDirty = true;
    return *this;
  }


  Camera& Camera::setWorldPos(glm::vec3 const& newWorld) {
    if (getParent() && getParent()->getTransform())
      m_localPos = newWorld - getParent()->getTransform()->getPosition();
    else
      m_localPos = newWorld;

    m_viewDirty = true;
    return *this;
  }

  Camera& Camera::setYaw(float newYaw) {
    m_yaw = newYaw;
    m_viewDirty = true;
    return *this;
  }

  Camera& Camera::setPitch(float newPitch) {
    m_pitch     = glm::clamp(newPitch, glm::radians(-89.f), glm::radians(89.f));
    m_viewDirty = true;
    return *this;
  }

  Camera& Camera::addYaw(float plusYaw) {
    return setYaw(m_yaw + plusYaw);
  }

  Camera& Camera::addPitch(float plusPitch) {
    return setPitch(m_pitch + plusPitch);
  }

  Camera& Camera::lookAt(glm::vec3 const& look) {
    glm::vec3 vec = glm::normalize(look - getWorldPos());
    m_yaw = atan2f(vec.x, -vec.y);
    m_pitch = atan2f(sqrt(vec.x * vec.x + vec.y * vec.y), vec.z);
    m_viewDirty = true;
    return *this;
  }

  ////////////////////
  // GETTERS
  //////////
  // PROJECTION
  float Camera::getAspect() const {
    return m_aspect;
  }

  float Camera::getFOV() const {
    return m_fov;
  }

  float Camera::getNear() const {
    return m_near;
  }

  float Camera::getFar() const {
    return m_far;
  }

  //////////
  // DIRECTION
  glm::vec3 Camera::getForward() const {
    // we dont tilt the camera, so no roll
    // the following is true if Y is up: it is not up in this engine, Z is.
    /*return {  cos(m_yaw) * cos(m_pitch),
              sin(m_pitch),
              sin(m_yaw) * cos(m_pitch)
    };*/
    return { cos(-m_yaw) * cos(m_pitch),
              sin(-m_yaw) * cos(m_pitch),
              sin(m_pitch)
    };
  }
  
  glm::vec3 Camera::getRight() const {
    // note: negative here due to +Z being up
    return -normalize(cross(getWorldUp(), getForward()));
  }
  
  glm::vec3 Camera::getUp() const {
    return normalize(cross(getForward(), getRight()));
  }
  
  //////////
  // VIEW
  glm::vec3 const& Camera::getWorldUp() const {
    return m_worldUp;
  }
  
  glm::vec3 const& Camera::getLocalPos() const {
    return m_localPos;
  }
  
  glm::vec3 Camera::getWorldPos() const {
    return getParent() && getParent()->getTransform()
             ? m_localPos + getParent()->getTransform()->getPosition()
             : m_localPos;
  }
  
  float Camera::getYaw() const {
    return m_yaw;
  }

  float Camera::getPitch() const {
    return m_pitch;
  }
}