// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : Application.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 13d
// * Last Altered: 2019y 09m 13d
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

#ifndef DW_APPLICATION_H
#define DW_APPLICATION_H

#include "render/Renderer.h"
#include "render/MeshManager.h"
#include "obj/Camera.h"
#include "obj/Material.h"

#include <chrono>
#include <memory>

namespace dw {
  class Application {
  public:
    Application();

    int parseCommandArgs(int argc, char** argv);
    int run();

  private:
    int initialize();
    int loop();

    int shutdown();

    // TIME VARIABLES
    using ClockType = std::chrono::high_resolution_clock;
    using DurationType = std::chrono::duration<float, std::chrono::seconds::period>;
    ClockType::time_point m_startTime;

    // SCENE VARIABLES
    util::ptr<Scene> m_curScene{ nullptr };

    std::vector<util::ptr<Scene>> m_scenes;
    util::ptr<Scene> m_mainScene{ nullptr };

    util::ptr<Scene> createPushScene();

    util::ptr<Renderer> m_renderer{ nullptr };
    InputHandler* m_inputHandler{ nullptr };
    GLFWWindow* m_window{ nullptr };
    TextureManager m_textureManager;
    MaterialManager m_mtlManager;
    MeshManager m_meshManager;
    Renderer::ShaderControl m_shaderControl {};
    bool m_resizedWindow{ false };
  };
} // namespace dw
#endif
