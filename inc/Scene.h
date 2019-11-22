// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Scene.h
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

#ifndef DW_SCENE_H
#define DW_SCENE_H

#include "Utils.h"

#include "Camera.h"

#include <vector>

namespace dw {
  class Object;
  class Light;
  class ShadowedLight;

  class Scene {
  public:
    using ObjContainer = std::vector<util::ptr<Object>>;
    using LightContainer = std::vector<util::ptr<Light>>;

    Scene& addObject(util::ptr<Object> object);
    Scene& addLight(util::ptr<Light> light);
    Scene& addGlobalLight(ShadowedLight const& light);
    Scene& setCamera(Camera const& camera);
    Scene& setBackground(util::ptr<Texture> bg, util::ptr<Texture> irradiance);

    NO_DISCARD ObjContainer const& getObjects() const;
    NO_DISCARD LightContainer const& getLights() const;
    NO_DISCARD std::vector<ShadowedLight> const& getGlobalLights() const;
    NO_DISCARD Camera const& getCamera() const;
    NO_DISCARD util::ptr<Texture> getBackground() const;
    NO_DISCARD util::ptr<Texture> getIrradiance() const;

    ObjContainer& getObjects();
    LightContainer& getLights();
    Camera& getCamera();

  private:
    util::ptr<Texture> m_background;
    util::ptr<Texture> m_backgroundIrradiance;
    ObjContainer m_objects;
    LightContainer m_lights;
    std::vector<ShadowedLight> m_globalLights;
    Camera m_camera;
  };
}

#endif
