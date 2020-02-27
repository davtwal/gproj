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

#include "util/MyMath.h"
#include "util/Utils.h"

#include "obj/Component.h"

namespace dw::obj {
  class Camera : public ComponentBase<Camera> {
  public:
    //Camera();
    NO_DISCARD Type getType() const override { return Type::ctCamera; }
    NO_DISCARD std::string getTypeName() const override { return "Camera"; }

    Camera& setAspect(float newAspect);
    Camera& setFOV(float newFOV);
    Camera& setNear(float newNear);
    Camera& setFar(float newFar);

    Camera& setWorldUp(glm::vec3 const& newUp);
    Camera& setLocalPos(glm::vec3 const& newLocal);
    Camera& addLocalPos(glm::vec3 const& plusLocal);

    // adjusts the local position such that the transform's position + local pos = newWorld
    Camera& setWorldPos(glm::vec3 const& newWorld);
    Camera& setYaw(float newYaw);
    Camera& setPitch(float newPitch);

    // used by input to do +=
    Camera& addYaw(float plusYaw);
    Camera& addPitch(float plusPitch);

    Camera& lookAt(glm::vec3 const& look);

    // Getters
    // Projection
    NO_DISCARD float getAspect() const;
    NO_DISCARD float getFOV() const;
    NO_DISCARD float getNear() const;
    NO_DISCARD float getFar() const;

    // View
    NO_DISCARD float getYaw() const;
    NO_DISCARD float getPitch() const;
    NO_DISCARD glm::vec3 const& getWorldUp() const;
    NO_DISCARD glm::vec3 const& getLocalPos() const;
    NO_DISCARD glm::vec3        getWorldPos() const;

    // Direction
    NO_DISCARD glm::vec3        getUp() const;
    NO_DISCARD glm::vec3        getRight() const;
    NO_DISCARD glm::vec3        getForward() const;

    // Transformation
    NO_DISCARD glm::mat4        worldToCamera();  // View matrix
    NO_DISCARD glm::mat4        cameraToWorld();  // Inverse view matrix
    NO_DISCARD glm::mat4 const& cameraToNDC();    // Projection matrix

    NO_DISCARD static glm::mat4 GetDefaultView();
    NO_DISCARD static glm::mat4 GetDefaultProj();

    // wow this lines up nicely mostly
    static constexpr float DEFAULT_ASPECT     = 1.f;
    static constexpr float DEFAULT_NEAR       = 0.1f;
    static constexpr float DEFAULT_FAR        = 100.f;
    static constexpr float DEFAULT_FOV        = glm::radians(90.f);
    static const glm::vec3 DEFAULT_LOOK_DIR;
    static const glm::vec3 DEFAULT_WORLD_UP;
    static const glm::vec3 DEFAULT_RIGHT_DIR;

  private:
    glm::mat4 m_viewMtx {};
    glm::mat4 m_projMtx {};

    glm::vec3 m_worldUp{ DEFAULT_WORLD_UP };
    glm::vec3 m_localPos{ 0.f };

    bool m_viewDirty{ true };
    bool m_projDirty{ true };

    float m_yaw{ 0.f };
    float m_pitch{ 0.f };
    float m_fov{ DEFAULT_FOV };
    float m_aspect{ DEFAULT_ASPECT };
    float m_near{ DEFAULT_NEAR };
    float m_far{ DEFAULT_FAR };
  };
}
//
//namespace dw {
//  class Camera {
//  public:
//    Camera();
//
//    static inline const glm::vec3 UP_DIR = { 0, 0.f, 1.f };
//
//    glm::mat4 const& getView();
//    glm::mat4 const& getProj();
//    NO_DISCARD glm::vec3 const& getEyePos() const;
//    NO_DISCARD glm::vec3 const& getViewDir() const;
//
//    NO_DISCARD glm::vec3 getRightDir() const;
//    NO_DISCARD glm::vec3 getUpDir() const;
//
//    NO_DISCARD float getAspect() const;
//    NO_DISCARD float getFOV() const;
//    NO_DISCARD float getFOVDeg() const;
//    NO_DISCARD float getNearDist() const;
//    NO_DISCARD float getFarDist() const;
//
//    //Camera& setDimensions(float width, float height);
//    Camera& setNearDepth(float near);
//    Camera& setFarDepth(float far);
//    Camera& setEyePos(glm::vec3 const& pos);
//    Camera& setViewDir(glm::vec3 const& dir);
//    Camera& setFOV(float radians);
//    Camera& setFOVDeg(float degrees);
//
//    // aspect = width / height;
//    Camera& setAspect(float aspect);
//
//    // changes the view direction to look at a specificpoint
//    Camera& setLookAt(glm::vec3 const& look);
//
//  private:
//
//    //float m_width = 10;
//    //float m_height = 10;
//    float m_aspect = 1;
//    float m_fov = 45.f;
//    float m_nearDist = 0.1f;
//    float m_farDist = 100.f;
//
//    glm::vec3 m_eyePos {0, 0, 0};
//    glm::vec3 m_viewDir {0, 0, -1};
//    glm::mat4 m_view{1.f};
//    glm::mat4 m_proj{1.f};
//    bool m_viewDirty{ true };
//    bool m_projDirty{ true };
//  };
//}

#endif
