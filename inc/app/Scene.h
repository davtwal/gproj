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


#include "render/Texture.h"
#include "util/Utils.h"
#include "obj/Camera.h"
#include "obj/Object.h"
#include "obj/Light.h"
//#include "render/Renderer.h"

#include <vector>

namespace dw {
  using namespace obj;
  /*class Scene2 {
  public:
    using ObjContainer = std::vector<util::ptr<Object>>;
    using LightContainer = std::vector<util::ptr<Light>>;



  };*/

  class obj::Object;
  class Light;
  class ShadowedLight;
  class Renderer;
  //class Renderer::ShaderControl;
  class InputHandler;

  class Scene {
  public:
    using MeshPtr = util::ptr<Mesh>;

    using ObjContainer    = std::vector<util::ptr<obj::Object>>;
    using LightContainer  = std::vector<util::ptr<Light>>;

    //virtual void update(InputHandler* input, float t, float dt) = 0;

    Scene& addObject(ObjContainer::value_type const& object);
    Scene& addLight(LightContainer::value_type const& light);
    Scene& addGlobalLight(ShadowedLight const& light);

    Scene& setCamera(util::ptr<obj::Camera> camera);
    Scene& setBackground(util::ptr<Texture> bg, util::ptr<Texture> irradiance);
    //Scene& setControl(Renderer::ShaderControl* control);

    //Renderer::ShaderControl * getControl();
    NO_DISCARD ObjContainer const& getObjects() const;
    NO_DISCARD LightContainer const& getLights() const;
    NO_DISCARD std::vector<ShadowedLight> const& getGlobalLights() const;
    //NO_DISCARD Camera const& getCamera() const;
    NO_DISCARD util::ptr<Texture> getBackground() const;
    NO_DISCARD util::ptr<Texture> getIrradiance() const;

    ObjContainer& getObjects();
    LightContainer& getLights();
    NO_DISCARD util::ptr<obj::Camera> getCamera() const;

  private:
    //Renderer::ShaderControl m_control;
    util::ptr<Texture> m_background;
    util::ptr<Texture> m_backgroundIrradiance;
    ObjContainer m_objects;
    LightContainer m_lights;
    std::vector<ShadowedLight> m_globalLights;
    util::ptr<obj::Camera> m_camera {nullptr};
  };

  /*class LevelScene : public Scene {
  public:
    void buildScene(util::ptr<Object> skydome, float windowAspect,
                    util::ptr<Mesh> groundPlane,
                    util::ptr<Mesh> teapotMesh,
                    util::ptr<Mesh> cubeMesh,
                    util::ptr<Mesh> lampMesh);

    void update(InputHandler* input, float t, float dt) override;
  };

  class PushScene : public Scene {
  public:
    void buildScene(util::ptr<Object> skydome, float windowAspect,
      util::ptr<Mesh> groundPlane,
      util::ptr<Mesh> teapotMesh,
      util::ptr<Mesh> cubeMesh,
      util::ptr<Mesh> lampMesh);

    void update(InputHandler* input, float t, float dt) override;
  };

  class RotateScene : public Scene {
  public:
    void buildScene(util::ptr<Object> skydome, float windowAspect,
      util::ptr<Mesh> groundPlane,
      util::ptr<Mesh> teapotMesh,
      util::ptr<Mesh> cubeMesh,
      util::ptr<Mesh> lampMesh);

    void update(InputHandler* input, float t, float dt) override;
  };*/
}

#endif
