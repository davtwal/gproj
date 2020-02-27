// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Scene.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 11m 15d
// * Last Altered: 2019y 11m 15d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#include "app/Scene.h"

#include "obj/Object.h"
#include "obj/Light.h"

namespace dw {
  Scene& Scene::addObject(ObjContainer::value_type const& object) {
    m_objects.push_back(object);
    return *this;
  }

  Scene& Scene::addLight(LightContainer::value_type const& light) {
    m_lights.push_back(light);
    return *this;
  }

  Scene& Scene::addGlobalLight(ShadowedLight const& light) {
    m_globalLights.push_back(light);
    return *this;
  }

  Scene& Scene::setCamera(std::shared_ptr<obj::Camera> camera) {
    m_camera = camera;
    return *this;
  }

  Scene& Scene::setBackground(util::ptr<Texture> bg, util::ptr<Texture> irradiance) {
    m_background = bg;
    m_backgroundIrradiance = irradiance;
    return *this;
  }

  Scene::ObjContainer const& Scene::getObjects() const {
    return m_objects;
  }

  Scene::LightContainer const& Scene::getLights() const {
    return m_lights;
  }

  Scene::ObjContainer& Scene::getObjects() {
    return m_objects;
  }

  Scene::LightContainer& Scene::getLights() {
    return m_lights;
  }

  std::vector<ShadowedLight> const& Scene::getGlobalLights() const {
    return m_globalLights;
  }

  std::shared_ptr<obj::Camera> Scene::getCamera() const {
    return m_camera;
  }
  util::ptr<Texture> Scene::getBackground() const {
    return m_background;
  }

  util::ptr<Texture> Scene::getIrradiance() const {
    return m_backgroundIrradiance;
  }
}